#pragma once
#include"common.h"
#define LOOPVAL 10

/*
P(s), sem_wait(s):
    s.value = s.value - 1 
    if (s.value < 0)
        yield to other thread
    else
        continue

V(s), sem_post(s):
    s.value = s.value + 1
    continue
*/
namespace binary{
    void test(){
        sem_t sem;
        sem_init(&sem, 0, 1);   //init to 1

        sem_wait(&sem); //0
        //critical section
        sem_post(&sem); //1

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

    void printsem(const char* str, sem_t* sem){
        int semval = 0;
        sem_getvalue(sem, &semval);
        printf("%s = %d\n", str, semval);
    }

    void* child_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sem_post(&arg->sem);    //2
        printsem("sem", &arg->sem);
        return NULL;
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.sem, 0, 1);   //init to 1
        printsem("sem", &arg.sem);
        pthread_t child;
        printf("begin\n");
        pthread_create(&child, NULL, child_thread, &arg);
        sleep(1);
        sem_wait(&arg.sem);     //1
        printsem("sem", &arg.sem);
        printf("end\n");
        

    }

}

/*
these are "counting semaphore"s:
empty = 10
full = 0
used to control multiple instances

producer empty--, full++
consumer empty++, full--

result:
empty = 10
full = 0
*/
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
        sem_t lock;
    } real_lock_t;
    
    typedef struct{
        real_lock_t real_lock;
        buffer_t buffer;
    } arg_t;

    void printsem(const char* str, sem_t* sem){
        int semval = 0;
        sem_getvalue(sem, &semval);
        printf("%s = %d\n", str, semval);
    }

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
        //sleep(1);   //to get it evenly running for the test
        for(int i = 0; i < LOOPVAL; i++){
            printf("producer..."); printsem("sem(empty) wait", &arg->real_lock.empty);
            sem_wait(&arg->real_lock.empty);
            printf("producer..."); printsem("sem(empty) wait finished", &arg->real_lock.empty);
            //printsem("producer...lock", &arg->real_lock.lock);
            //sem_wait(&arg->real_lock.lock);
            //printsem("producer...lock", &arg->real_lock.lock);
            put(&arg->buffer, i);
            //sem_post(&arg->real_lock.lock);
            //printsem("producer...lock", &arg->real_lock.lock);
            printf("put: %d\n", i);
            printf("producer..."); printsem("sem(full) post", &arg->real_lock.full);
            sem_post(&arg->real_lock.full);
            printf("producer..."); printsem("sem(full) post finished", &arg->real_lock.full);
            //sched_yield();  //to get it evenly running for the test
        }
        printf("producer finished.\n");
        printsem("empty", &arg->real_lock.empty);
        printsem("full", &arg->real_lock.full);
    }

    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        int temp = 0;
        while(temp != LOOPVAL - 1){ //producer ends at LOOPVAL - 1
            printf("consumer..."); printsem("sem(full) wait", &arg->real_lock.full);
            sem_wait(&arg->real_lock.full);
            printf("consumer..."); printsem("sem(full) wait finished", &arg->real_lock.full);
            //printsem("consumer...lock", &arg->real_lock.lock);
            //sem_wait(&arg->real_lock.lock);
            //printsem("consumer...lock", &arg->real_lock.lock);
            temp = get(&arg->buffer);
            //sem_post(&arg->real_lock.lock);
            //printsem("consumer...lock", &arg->real_lock.lock);
            printf("get: %d\n", temp);
            printf("consumer..."); printsem("sem(empty) post", &arg->real_lock.empty);
            sem_post(&arg->real_lock.empty);
            printf("consumer..."); printsem("sem(empty) post finished", &arg->real_lock.empty);
        }
        printf("consumer finished.\n");
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.real_lock.empty, 0, MAX);
        sem_init(&arg.real_lock.full, 0, 0);
        sem_init(&arg.real_lock.lock, 0, 1);
        printsem("empty", &arg.real_lock.empty);
        printsem("full", &arg.real_lock.full);
        printsem("lock", &arg.real_lock.lock);

        pthread_t producer_a = 0, producer_b = 0, consumer = 0;
        printf("begin\n");
        pthread_create(&producer_a, NULL, producer_thread, &arg);
        pthread_create(&producer_b, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);

        pthread_join(producer_a, NULL);
        pthread_join(producer_b, NULL);
        pthread_join(consumer, NULL);
        printf("end\n");

        printsem("empty", &arg.real_lock.empty);
        printsem("full", &arg.real_lock.full);
        printsem("lock", &arg.real_lock.lock);
        
    }

}

//above example ignores mutual exclusion
//lock to prevent data race
//"binary semaphore": mutex lock
namespace putandget_mutex{
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
        sem_t lock;
    } real_lock_t;
    
    typedef struct{
        real_lock_t real_lock;
        buffer_t buffer;
    } arg_t;

    void printsem(const char* str, sem_t* sem){
        int semval = 0;
        sem_getvalue(sem, &semval);
        printf("%s = %d\n", str, semval);
    }

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
        //sleep(1);   //to get it evenly running for the test
        for(int i = 0; i < LOOPVAL; i++){
            printf("producer..."); printsem("sem(empty) wait", &arg->real_lock.empty);
            sem_wait(&arg->real_lock.empty);
            printf("producer..."); printsem("sem(empty) wait finished", &arg->real_lock.empty);
            printsem("producer...lock", &arg->real_lock.lock);
            sem_wait(&arg->real_lock.lock);
            printsem("producer...lock", &arg->real_lock.lock);
            put(&arg->buffer, i);
            sem_post(&arg->real_lock.lock);
            printsem("producer...lock", &arg->real_lock.lock);
            printf("put: %d\n", i);
            printf("producer..."); printsem("sem(full) post", &arg->real_lock.full);
            sem_post(&arg->real_lock.full);
            printf("producer..."); printsem("sem(full) post finished", &arg->real_lock.full);
            //sched_yield();  //to get it evenly running for the test
        }
        printf("producer finished.\n");
        printsem("empty", &arg->real_lock.empty);
        printsem("full", &arg->real_lock.full);
    }

    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        int temp = 0;
        while(temp != LOOPVAL - 1){ //producer ends at LOOPVAL - 1
            printf("consumer..."); printsem("sem(full) wait", &arg->real_lock.full);
            sem_wait(&arg->real_lock.full);
            printf("consumer..."); printsem("sem(full) wait finished", &arg->real_lock.full);
            printsem("consumer...lock", &arg->real_lock.lock);
            sem_wait(&arg->real_lock.lock);
            printsem("consumer...lock", &arg->real_lock.lock);
            temp = get(&arg->buffer);
            sem_post(&arg->real_lock.lock);
            printsem("consumer...lock", &arg->real_lock.lock);
            printf("get: %d\n", temp);
            printf("consumer..."); printsem("sem(empty) post", &arg->real_lock.empty);
            sem_post(&arg->real_lock.empty);
            printf("consumer..."); printsem("sem(empty) post finished", &arg->real_lock.empty);
        }
        printf("consumer finished.\n");
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.real_lock.empty, 0, MAX);
        sem_init(&arg.real_lock.full, 0, 0);
        sem_init(&arg.real_lock.lock, 0, 1);
        printsem("empty", &arg.real_lock.empty);
        printsem("full", &arg.real_lock.full);
        printsem("lock", &arg.real_lock.lock);

        pthread_t producer_a = 0, producer_b = 0, consumer = 0;
        printf("begin\n");
        pthread_create(&producer_a, NULL, producer_thread, &arg);
        pthread_create(&producer_b, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);

        pthread_join(producer_a, NULL);
        pthread_join(producer_b, NULL);
        pthread_join(consumer, NULL);
        printf("end\n");

        printsem("empty", &arg.real_lock.empty);
        printsem("full", &arg.real_lock.full);
        printsem("lock", &arg.real_lock.lock);
        
    }

}

