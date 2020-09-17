#pragma once
#include"common.h"
#define LOOPVAL 10

namespace simplethreadjoin{

    typedef struct{
        int result;
    } arg_t;

    void* child_func(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        arg->result = 1;
        return NULL;
    }

    void parent_func(pthread_t* child, arg_t* arg){
        sched_getaffinity(0, 0, NULL);

        pthread_create(child, NULL, child_func, arg);
        while(!arg->result);    //spin(like join)
        printf("finished\n");
    }

    void test(){
        pthread_t child = 0;
        arg_t arg = {0};
        parent_func(&child, &arg);

    }

}

/*
two cases:
1. thread_join waits for the signal
or
2. thread_exit already happened(result = 1), thread_join's while loop catches it and dont execute wait()
(in case #2, if there was no while loop flag check, it would wait() for signal forever)
*/
namespace waitforthread{
    typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t cond;
    } real_lock_t;
    typedef struct{
        real_lock_t real_lock;
        int result;
    } arg_t;

    void thread_exit(arg_t* arg){
        pthread_mutex_lock(&arg->real_lock.lock);
        arg->result = 1;
        pthread_cond_signal(&arg->real_lock.cond);
        pthread_mutex_unlock(&arg->real_lock.lock);
    }
    void thread_join(arg_t* arg){
        pthread_mutex_lock(&arg->real_lock.lock);
        //sleep(1);
        while(!arg->result)
            pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
        pthread_mutex_unlock(&arg->real_lock.lock);
    }


    void* child_func(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        thread_exit(arg);

        return NULL;
    }

    void parent_func(pthread_t* child, arg_t* arg){
        sched_getaffinity(0, 0, NULL);

        pthread_create(child, NULL, child_func, arg);
        thread_join(arg);
        printf("finished\n");
    }
    void test(){
        pthread_t child = 0;
        arg_t arg = {0};
        parent_func(&child, &arg);
    }

}

/*
why this will not work:

on thread_join, flag is 0 and just before executing wait(),
child somehow interrupts parent's thread_join and finished thread_exit procedure.
when thread_join resumes, it is stuck on wait() with no signal to catch.

its not the if/while. always hold the lock on signal/wait.

*/
namespace nolockjoin{
    typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t cond;
    } real_lock_t;
    typedef struct{
        real_lock_t real_lock;
        int result;
    } arg_t;

    void thread_exit(arg_t* arg){
        //sleep(1);
        arg->result = 1;
        pthread_cond_signal(&arg->real_lock.cond);
    }
    void thread_join(arg_t* arg){
        if(!arg->result){   //same issue for while loop as well!
            //sleep(1);
            pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
        }
    }

    void* child_func(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);
        
        thread_exit(arg);

        return NULL;
    }

    void parent_func(pthread_t* child, arg_t* arg){
        sched_getaffinity(0, 0, NULL);

        pthread_create(child, NULL, child_func, arg);
        thread_join(arg);
        printf("finished\n");
    }
    void test(){
        pthread_t child = 0;
        arg_t arg = {0};
        parent_func(&child, &arg);
    }

}

namespace putandget{
     typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t cond;
    } real_lock_t;
    typedef struct{
        int value;
        int count;
    } buffer_t;
    typedef struct{
        real_lock_t real_lock;
        int result;
        buffer_t buffer;
    } arg_t;

    void put(buffer_t* buffer, int value){
        //only one allowed in buffer
        if(buffer->count == 0){
            buffer->count = 1;
            buffer->value = value;
        }
    }
    int get(buffer_t* buffer){
        if(buffer->count == 1){
            buffer->count = 0;
            return buffer->value;
        }
        return -1;
    }

    void* producer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        for(int i = 0; i < LOOPVAL; i++){
            pthread_mutex_lock(&arg->real_lock.lock);
            printf("producer...locked\n");
           
            //wait until get() happens on consumer thread
            if(arg->buffer.count == 1){
                printf("producer...wait cond\n");
                //wait() unlocks mutex and then locks again on receiving signal!
                //this is why the other thread is able to send signal with locks on
                pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
            }
            else{
                printf("producer...skip cond\n");
            }
            put(&arg->buffer, i);
            pthread_cond_signal(&arg->real_lock.cond);
            printf("producer...signal cond\n");
            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("producer...unlocked\n");
        }
    }
    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        for(int i = 0; i < LOOPVAL; i++){
            
            pthread_mutex_lock(&arg->real_lock.lock);

            printf("consumer...locked\n");
            if(arg->buffer.count == 0){
                printf("consumer...wait cond\n");
                //wait() unlocks mutex and then locks again on receiving signal!
                //this is why the other thread is able to send signal with locks on
                pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
                
            }
            else{
                printf("consumer...skip cond\n");
            }
            printf("get: %d\n", get(&arg->buffer));
            pthread_cond_signal(&arg->real_lock.cond);
            printf("consumer...signal cond\n");
            

            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("consumer...unlocked\n");
        }
    }

    void test(){
        pthread_t producer, consumer;
        producer = consumer = 0;
        arg_t arg = {0};
        

        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);
        
        pthread_join(producer, NULL);
        pthread_join(consumer, NULL);
    }

}