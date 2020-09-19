#pragma once
#include"common.h"
#define LOOPVAL 10

//example
namespace covering_conditions{
#define MAX_HEAP_SIZE 100

    typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t cond;
    } real_lock_t;

    typedef struct{
        int bytesLeft;
    } alloc_t;
    
    typedef struct{
        real_lock_t real_lock;
        alloc_t alloc;
        int result;
        int* arr[LOOPVAL];
    } arg_t;

    void* alloc(real_lock_t* real_lock, alloc_t* alloc, int size){
        pthread_mutex_lock(&real_lock->lock);
        while(alloc->bytesLeft < size){
            pthread_cond_wait(&real_lock->cond, &real_lock->lock);
        }
        void* ptr = malloc(size);
        alloc->bytesLeft -= size;
        pthread_mutex_unlock(&real_lock->lock);
        return ptr;
    }

    void dealloc(real_lock_t* real_lock, alloc_t* alloc, void* ptr, int size){
        pthread_mutex_lock(&real_lock->lock);
        alloc->bytesLeft += size;
        free(ptr);
        pthread_cond_signal(&real_lock->cond);
        pthread_mutex_unlock(&real_lock->lock);
    }

    void* producer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);
        sleep(1);
        for(int i = 0; i < LOOPVAL; i++){
            printf("producer\n");
            arg->arr[i] = (int*)alloc(&arg->real_lock, &arg->alloc, sizeof(int));
            *arg->arr[i] = i * 10;
            sched_yield();
        }
    }
    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            while(!arg->arr[i]) sched_yield();  //dont let dealloc happen first
            printf("consumer\n");
            printf("%d\n", *arg->arr[i]);
            dealloc(&arg->real_lock, &arg->alloc, arg->arr[i], sizeof(int));
        }
    }

    void test(){
        pthread_t producer = 0, consumer = 0;
        arg_t arg = {0};
        arg.alloc.bytesLeft = MAX_HEAP_SIZE;

        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);
        
        pthread_join(producer, NULL);
        pthread_join(consumer, NULL);
    }

}

//broadcast wakes all waiting threads
namespace covering_conditions_broadcast{
#define MAX_HEAP_SIZE 100

    typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t cond;
    } real_lock_t;

    typedef struct{
        int bytesLeft;
    } alloc_t;
    
    typedef struct{
        real_lock_t real_lock;
        alloc_t alloc;
        int result;
        int* arr[LOOPVAL];
    } arg_t;

    void* alloc(real_lock_t* real_lock, alloc_t* alloc, int size){
        pthread_mutex_lock(&real_lock->lock);
        while(alloc->bytesLeft < size){
            pthread_cond_wait(&real_lock->cond, &real_lock->lock);
        }
        void* ptr = malloc(size);
        alloc->bytesLeft -= size;
        pthread_mutex_unlock(&real_lock->lock);
        return ptr;
    }

    void dealloc(real_lock_t* real_lock, alloc_t* alloc, void* ptr, int size){
        pthread_mutex_lock(&real_lock->lock);
        alloc->bytesLeft += size;
        free(ptr);
        pthread_cond_broadcast(&real_lock->cond);   //broadcast
        pthread_mutex_unlock(&real_lock->lock);
    }

    void* producer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);
        sleep(1);
        for(int i = 0; i < LOOPVAL; i++){
            printf("producer\n");
            arg->arr[i] = (int*)alloc(&arg->real_lock, &arg->alloc, sizeof(int));
            *arg->arr[i] = i * 10;
        }
    }
    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            printf("consumer\n");
            printf("%d\n", *arg->arr[i]);
            dealloc(&arg->real_lock, &arg->alloc, arg->arr[i], sizeof(int));
        }
    }

    void test(){
        pthread_t producer = 0, consumer = 0;
        arg_t arg = {0};
        arg.alloc.bytesLeft = MAX_HEAP_SIZE;

        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);
        
        pthread_join(producer, NULL);
        pthread_join(consumer, NULL);
    }

}