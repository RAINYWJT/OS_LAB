#include "life.h"
#include <pthread.h>
#include <semaphore.h>

#define MODE1 0
#define MODE2 1
typedef struct tpool{
    int terminate;
    sem_t ready;
    pthread_cond_t wake;
    pthread_mutex_t mutex;
    LifeBoard* state;
    LifeBoard* next_state;
    pthread_t *threads;
}ThreadPool; 
typedef struct compound{
    ThreadPool*pool;
    int x1,x2,y1,y2;
}Compound;
void *func(void*com){
    Compound*mess = (Compound*)com;
    int x1 = mess->x1;
    int x2 = mess->x2;
    int y1 = mess->y1;
    int y2 = mess->y2;
    ThreadPool* pool = mess->pool;
    LifeBoard*state = pool->state;
    LifeBoard*next_state = pool->next_state;
    int width = state->width;
    while(pool->terminate){
        for (int y = y1; y < y2; y++) {
            for (int x = x1; x < x2; x++) {
                int live_neighbors = 0;
                live_neighbors += state->cells[(y-1)*width+x-1]+state->cells[(y-1)*width+x]+state->cells[(y-1)*width+x+1]+state->cells[y*width+x-1]+state->cells[y*width+x]+state->cells[y*width+x+1]+state->cells[(y+1)*width+x-1]
                +state->cells[(y+1)*width+x]+state->cells[(y+1)*width+x+1];
                LifeCell current_cell = state->cells[y*width + x];
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
                next_state->cells[y * width + x] = new_cell;
            }
        }
        pthread_mutex_lock(&pool->mutex);
        sem_post(&pool->ready);
        pthread_cond_wait(&pool->wake,&pool->mutex);
        pthread_mutex_unlock(&pool->mutex);
    }
    pthread_exit(NULL);
}

void simulate_life_parallel(int threads, LifeBoard *state, int steps) {
    if (steps == 0) return;
    LifeBoard*next_state = create_life_board(state->width, state->height);
    ThreadPool pool;
    pool.terminate = 1;
    pool.state = state;
    pool.next_state = next_state;
    int width = state->width-2;
    int height = state->height-2;
    int mode;
    if(height>=threads){
        mode = MODE1;
    }
    else if(width*height>=threads){
        mode = MODE2;
    }
    else{
        threads = width*height;
        mode = MODE2;
    }
    sem_init(&pool.ready,0,0);
    pthread_mutex_init(&pool.mutex,NULL);
    pthread_cond_init(&pool.wake,NULL);
    pool.threads = (pthread_t*)malloc(sizeof(pthread_t)*threads);
    Compound* com = (Compound*)malloc(sizeof(Compound)*threads);
    if(mode == MODE1){
        int len = height/threads;
        for(int i = 0;i < threads;i++){
            com[i].pool = &pool;
            com[i].x1 = 1;
            com[i].x2 = width+1;
            com[i].y1 = 1 + len*i;
            if(i == threads-1){
                com[i].y2 = height+1;
            }
            else{
                com[i].y2 = com[i].y1 + len;
            }
            pthread_create(pool.threads+i,NULL,func,(void*)(com+i));
        }
    }
    else{
        int before = threads%height;
        int div_line = threads/height;
        int line_len = width/(div_line+1);
        int id = 0;
        for(int i = 0;i < before;i++){
            int pos = 1;
            for(int j = 0;j < div_line+1;j++){
                com[id].pool = &pool;
                com[id].x1 = pos;
                if(j == div_line){
                    com[id].x2 = width+1;
                }
                else{
                    com[id].x2 = pos + line_len;
                }
                pos+=line_len;
                com[id].y1 = i + 1;
                com[id].y2 = i + 2;
                pthread_create(pool.threads+id,NULL,func,(void*)(com+id));
                id+=1;
            }
        }
        line_len = width/div_line;
        for(int i = before;i < height;i++){
            int pos = 1;
            for(int j = 0;j < div_line;j++){
                com[id].pool = &pool;
                com[id].x1 = pos;
                if(j == div_line-1){
                    com[id].x2 = width+1;
                }
                else{
                    com[id].x2 = pos + line_len;
                }
                pos+=line_len;
                com[id].y1 = i + 1;
                com[id].y2 = i + 2;
                pthread_create(pool.threads+id,NULL,func,(void*)(com+id));
                id+=1;
            }
        }
    } 
    for(int i = 0;i < steps;i++){
        for(int j = 0;j < threads;j++){
            sem_wait(&pool.ready);
        }
        swap(state,next_state);
        if(i==steps-1){
           pool.terminate = 0; 
        }
        pthread_mutex_lock(&pool.mutex);
        pthread_cond_broadcast(&pool.wake);
        pthread_mutex_unlock(&pool.mutex);
    }
    for(int i = 0;i < threads;i++){
        pthread_join(pool.threads[i],NULL);
    }
    destroy_life_board(next_state);
}