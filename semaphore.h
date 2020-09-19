#pragma once
#include"common.h"
#define LOOPVAL 10

namespace binary{
    void test(){
        sem_t sem;
        sem_init(&sem, 0, 1);   //init to 1

        sem_wait(&sem);
        //critical section
        sem_post(&sem);

    }

}

namespace parentchild{

    typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t cond;
    } real_lock_t;
    
    typedef struct{
        real_lock_t real_lock;
        sem_t sem;
    } arg_t;

    void* child_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sem_post(&arg->sem);
        return NULL;
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.sem, 0, 1);   //init to 1
        pthread_t child;
        printf("begin\n");
        pthread_create(&child, NULL, child_thread, &arg);
        sem_wait(&arg.sem);
        printf("end\n");
        

    }

}

namespace putandget{
#define MAX 10
    typedef struct{
        int arr[MAX];
        int fill_index;
        int use_index;
        int count;
    } buffer_t;

    typedef struct{
        sem_t empty;
        sem_t full;
    } real_lock_t;
    
    typedef struct{
        real_lock_t real_lock;
        buffer_t buffer;
    } arg_t;

    void put(buffer_t* buffer, int value){
        buffer->arr[buffer->fill_index] = value;
        buffer->fill_index = (buffer->fill_index + 1) % MAX;
        buffer->count++;
    }
    int get(buffer_t* buffer){
        if(buffer->count == 0) return -1;
        int temp = buffer->arr[buffer->use_index];
        buffer->use_index = (buffer->use_index + 1) % MAX;
        buffer->count--;
        return temp;

    }

    void* producer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sleep(1);
        for(int i = 0; i < LOOPVAL; i++){
            printf("producer... sem(empty) wait\n");
            sem_wait(&arg->real_lock.empty);
            printf("producer... sem(empty) wait finished\n");
            put(&arg->buffer, i);
            printf("put: %d\n", i);
            printf("producer... sem(full) post\n");
            sem_post(&arg->real_lock.full);
            printf("producer... sem(full) post finished\n");
            sched_yield();
        }
        printf("producer finished.\n");
    }

    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        int temp = 0;
        while(temp != LOOPVAL - 1){ //producer ends at LOOPVAL - 1
            printf("consumer... sem(full) wait\n");
            sem_wait(&arg->real_lock.full);
            printf("consumer... sem(full) wait finished\n");
            temp = get(&arg->buffer);
            printf("get: %d\n", temp);
            printf("consumer... sem(empty) post\n");
            sem_post(&arg->real_lock.empty);
            printf("consumer... sem(empty) post finished\n");
        }
        printf("consumer finished.\n");
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.real_lock.empty, 0, MAX);
        sem_init(&arg.real_lock.full, 0, 0);
        pthread_t producer = 0, consumer = 0;
        printf("begin\n");
        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);

        pthread_join(producer, NULL);
        pthread_join(consumer, NULL);
        printf("end\n");
        
    }

}

