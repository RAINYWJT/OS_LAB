#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "life.h"

typedef struct {
    int start_row;
    int end_row;
    LifeBoard *state;
    LifeBoard *next_state;
    sem_t *semaphore;
} ThreadArg;

// 线程工作函数
void *thread_function(void *arg) {
    ThreadArg *thread_arg = (ThreadArg *)arg;
    int start_row = thread_arg->start_row;
    int end_row = thread_arg->end_row;
    LifeBoard *state = thread_arg->state;
    LifeBoard *next_state = thread_arg->next_state;

    for (int y = start_row; y < end_row; y++) {
        for (int x = 0; x < state->width; x++) {
            int live_neighbors = count_live_neighbors(state, x, y);
            LifeCell current_cell = at(state, x, y);
            LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
            set_at(next_state, x, y, new_cell);
        }
    }
    sem_post(thread_arg->semaphore);
    return NULL;
}

// 并行模拟生命游戏
void simulate_life_parallel(int threads, LifeBoard *state, int steps) {
    pthread_t *thread_handles = malloc(threads * sizeof(pthread_t));
    ThreadArg *thread_args = malloc(threads * sizeof(ThreadArg));
    LifeBoard *next_state = create_life_board(state->width, state->height);
    sem_t semaphore;

    // 初始化信号量
    sem_init(&semaphore, 0, 0);

    for (int step = 0; step < steps; step++) {
        for (int i = 0; i < threads; i++) {
            thread_args[i].start_row = i * (state->height / threads);
            thread_args[i].end_row = (i == threads - 1) ? state->height : (i + 1) * (state->height / threads);
            thread_args[i].state = state;
            thread_args[i].next_state = next_state;
            thread_args[i].semaphore = &semaphore;

            pthread_create(&thread_handles[i], NULL, thread_function, &thread_args[i]);
        }

        for (int i = 0; i < threads; i++) {
            sem_wait(&semaphore);
        }
        swap(state, next_state);
        
        for (int i = 0; i < threads; i++) {
            pthread_join(thread_handles[i], NULL);
        }
    }
    destroy_life_board(next_state);
    free(thread_handles);
    free(thread_args);
    sem_destroy(&semaphore);
}
