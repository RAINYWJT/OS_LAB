#include "life.h"
#include <pthread.h>
#include <semaphore.h>

typedef struct tpool {
    int flag;
    sem_t ready;
    pthread_cond_t wake;
    pthread_mutex_t mutex;
    LifeBoard* state;
    LifeBoard* next_state;
    pthread_t* threads;
} ThreadPool;

typedef struct compound {
    ThreadPool* pool;
    int start_row;
    int end_row;
} Compound;

void* func(void* com) {
    Compound* pound = (Compound*)com;
    int start_row = pound->start_row;
    int end_row = pound->end_row;
    ThreadPool* pool = pound->pool;
    LifeBoard* state = pool->state;
    LifeBoard* next_state = pool->next_state;
    int width = state->width;
    while (pool->flag) {
        for (int y = start_row; y < end_row; y++) {
            for (int x = 1; x < width - 1; x++) {
                int live_neighbors = 0;
                live_neighbors += state->cells[(y-1)*width+x-1]+
                                state->cells[(y-1)*width+x]+
                                state->cells[(y-1)*width+x+1]+
                                state->cells[y*width+x-1]+
                                state->cells[y*width+x]+
                                state->cells[y*width+x+1]+
                                state->cells[(y+1)*width+x-1]+
                                state->cells[(y+1)*width+x]+
                                state->cells[(y+1)*width+x+1];
                LifeCell current_cell = at(state, x, y);
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
                next_state->cells[y * next_state->width + x] = new_cell;
            }
        }
        pthread_mutex_lock(&pool->mutex);
        sem_post(&pool->ready);
        pthread_cond_wait(&pool->wake, &pool->mutex);
        pthread_mutex_unlock(&pool->mutex);
    }
    pthread_exit(NULL);
}


void simulate_life_parallel(int threads, LifeBoard* state, int steps) {
    if (steps == 0) {
        return;
    }
    LifeBoard* next_state = create_life_board(state->width, state->height);
    ThreadPool pool;
    pool.flag = 1;
    pool.state = state;
    pool.next_state = next_state;
    int height = state->height - 2;
    int rows_per_thread = height / threads;
    sem_init(&pool.ready, 0, 0);
    pthread_mutex_init(&pool.mutex, NULL);
    pthread_cond_init(&pool.wake, NULL);

    pool.threads = (pthread_t*)malloc(sizeof(pthread_t) * threads);
    Compound* com = (Compound*)malloc(sizeof(Compound) * threads);

    for (int i = 0; i < threads; i++) {
        com[i].pool = &pool;
        com[i].start_row = 1 + i * rows_per_thread;
        com[i].end_row = (i == threads - 1) ? height + 1 : com[i].start_row + rows_per_thread;
        pthread_create(pool.threads + i, NULL, func, (void*)(com + i));
    }

    for (int i = 0; i < steps; i++) {
        for (int j = 0; j < threads; j++) {
            sem_wait(&pool.ready);
        }
        swap(state, next_state);
        if (i == steps - 1) {
            pool.flag = 0;
        }
        pthread_mutex_lock(&pool.mutex);
        pthread_cond_broadcast(&pool.wake);
        pthread_mutex_unlock(&pool.mutex);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(pool.threads[i], NULL);
    }

    free(pool.threads);
    free(com);

    destroy_life_board(next_state);
}
