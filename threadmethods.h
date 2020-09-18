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
            printf("--------------------------------\n");
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
            printf("put: %d\n", i);
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

//one cond wont be enough for more than two threads.
/*
consumer#1...locked
consumer#1...wait cond(unlocked)
producer...locked
producer...skip cond
put: 8
producer...signal cond
producer...unlocked
    >consumer#0 sneaks in...
consumer#0...locked
consumer#0...skip cond
get: 8          //consumer#0 steals data...
consumer#0...signal cond
consumer#0...unlocked
    >consumer#1 resumes after wait()...
consumer#1...receive cond(locked)
get: -1         //no data for consumer#1...
consumer#1...signal cond
consumer#1...unlocked
*/
namespace putandget_twoconsumers{
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
            printf("--------------------------------\n");
            pthread_mutex_lock(&arg->real_lock.lock);
            printf("producer...locked\n");
           
            //wait until get() happens on consumer thread
            if(arg->buffer.count == 1){
                printf("producer...wait cond(unlocked)\n");
                //wait() unlocks mutex and then locks again on receiving signal!
                //this is why the other thread is able to send signal with locks on
                pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
                printf("producer...receive cond(locked)\n");
            }
            else{
                printf("producer...skip cond\n");
            }
            put(&arg->buffer, i);
            printf("put: %d\n", i);
            pthread_cond_signal(&arg->real_lock.cond);
            printf("producer...signal cond\n");
            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("producer...unlocked\n");
        }

        //inform end to consumers
        arg->result = 1;
        printf("ending producer.\n");
        
    }
    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        pid_t tid = gettid() % 2;

        for(int i = 0; !arg->result; i++){
            
            pthread_mutex_lock(&arg->real_lock.lock);

            printf("consumer#%d...locked\n", tid);
            if(arg->buffer.count == 0){
                printf("consumer#%d...wait cond(unlocked)\n", tid);
                //wait() unlocks mutex and then locks again on receiving signal!
                //this is why the other thread is able to send signal with locks on
                pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
                printf("consumer#%d...receive cond(locked)\n", tid);
            }
            else{
                printf("consumer#%d...skip cond\n", tid);
            }
            printf("get: %d\n", get(&arg->buffer));
            pthread_cond_signal(&arg->real_lock.cond);
            printf("consumer#%d...signal cond\n", tid);
            

            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("consumer#%d...unlocked\n", tid);
        }

        printf("ending consumer#%d.\n", tid);
    }

    void test(){
        pthread_t producer, consumer0, consumer1;
        producer = consumer0 = consumer1 = 0;
        arg_t arg = {0};
        

        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer0, NULL, consumer_thread, &arg);
        pthread_create(&consumer1, NULL, consumer_thread, &arg);
        
        pthread_join(producer, NULL);
        pthread_join(consumer0, NULL);
        pthread_join(consumer1, NULL);
    }

}

//put while loop
/*
producer...locked
producer...wait cond(unlocked)
consumer#1...receive cond(locked)
get: 8
consumer#1...signal cond
consumer#1...unlocked
consumer#1...locked
consumer#1...wait cond(unlocked)
producer...receive cond(locked)
put: 9
producer...signal cond
producer...unlocked
ending producer.
consumer#0...locked
consumer#0...skip cond
get: 9
consumer#0...signal cond
consumer#0...unlocked
ending consumer#0.
    > while loop makes sure that you wait again when data is not there.
consumer#1...receive cond(locked)
consumer#1...wait cond(unlocked)

no more data misses
*/
namespace putandget_mesasemantics{
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
            printf("--------------------------------\n");
            pthread_mutex_lock(&arg->real_lock.lock);
            printf("producer...locked\n");
           
            //wait until get() happens on consumer thread
            if(arg->buffer.count == 1){
                while(arg->buffer.count == 1){
                    printf("producer...wait cond(unlocked)\n");
                    //wait() unlocks mutex and then locks again on receiving signal!
                    //this is why the other thread is able to send signal with locks on
                    pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
                    printf("producer...receive cond(locked)\n");
                }
            }
            else{
                printf("producer...skip cond\n");
            }
            put(&arg->buffer, i);
            printf("put: %d\n", i);
            pthread_cond_signal(&arg->real_lock.cond);
            printf("producer...signal cond\n");
            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("producer...unlocked\n");
        }

        //inform end to consumers
        arg->result = 1;
        printf("ending producer.\n");
        
    }
    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        pid_t tid = gettid() % 2;

        for(int i = 0; !arg->result; i++){
            
            pthread_mutex_lock(&arg->real_lock.lock);

            printf("consumer#%d...locked\n", tid);
            if(arg->buffer.count == 0){
                while(arg->buffer.count == 0){
                    printf("consumer#%d...wait cond(unlocked)\n", tid);
                    //wait() unlocks mutex and then locks again on receiving signal!
                    //this is why the other thread is able to send signal with locks on
                    pthread_cond_wait(&arg->real_lock.cond, &arg->real_lock.lock);
                    printf("consumer#%d...receive cond(locked)\n", tid);
                }
            }
            else{
                printf("consumer#%d...skip cond\n", tid);
            }
            printf("get: %d\n", get(&arg->buffer));
            pthread_cond_signal(&arg->real_lock.cond);
            printf("consumer#%d...signal cond\n", tid);
            

            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("consumer#%d...unlocked\n", tid);
        }

        printf("ending consumer#%d.\n", tid);
    }

    void test(){
        pthread_t producer, consumer0, consumer1;
        producer = consumer0 = consumer1 = 0;
        arg_t arg = {0};
        

        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer0, NULL, consumer_thread, &arg);
        pthread_create(&consumer1, NULL, consumer_thread, &arg);
        
        pthread_join(producer, NULL);
        pthread_join(consumer0, NULL);
        pthread_join(consumer1, NULL);
    }

}

//use two conditions: empty, fill
//this will direct signal to proper whom to wake up
//prevent consumer#0 -> consumer#1
/*
producer...locked
producer...wait 'empty' cond(unlocked)
consumer#1...receive 'fill' cond(locked)
get: 8
consumer#1...signal 'empty' cond
consumer#1...unlocked
consumer#1...locked
consumer#1...wait 'fill' cond(unlocked)
producer...receive 'empty' cond(locked)
put: 9
producer...signal 'fill' cond
producer...unlocked
ending producer.
consumer#0...locked
consumer#0...skip cond
get: 9
consumer#0...signal 'empty' cond
consumer#0...unlocked
ending consumer#0.
consumer#1...receive 'fill' cond(locked)
consumer#1...wait 'fill' cond(unlocked)
*/
namespace putandget_twoconds{
     typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t empty;
        pthread_cond_t fill;
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
            printf("--------------------------------\n");
            pthread_mutex_lock(&arg->real_lock.lock);
            printf("producer...locked\n");
           
            //wait until get() happens on consumer thread
            if(arg->buffer.count == 1){
                while(arg->buffer.count == 1){
                    printf("producer...wait 'empty' cond(unlocked)\n");
                    //wait() unlocks mutex and then locks again on receiving signal!
                    //this is why the other thread is able to send signal with locks on
                    pthread_cond_wait(&arg->real_lock.empty, &arg->real_lock.lock);
                    printf("producer...receive 'empty' cond(locked)\n");
                }
            }
            else{
                printf("producer...skip cond\n");
            }
            put(&arg->buffer, i);
            printf("put: %d\n", i);
            pthread_cond_signal(&arg->real_lock.fill);
            printf("producer...signal 'fill' cond\n");
            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("producer...unlocked\n");
        }

        //inform end to consumers
        arg->result = 1;
        printf("ending producer.\n");
        
    }
    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        sched_getaffinity(0, 0, NULL);

        pid_t tid = gettid() % 2;

        for(int i = 0; !arg->result; i++){
            
            pthread_mutex_lock(&arg->real_lock.lock);

            printf("consumer#%d...locked\n", tid);
            if(arg->buffer.count == 0){
                while(arg->buffer.count == 0){
                    printf("consumer#%d...wait 'fill' cond(unlocked)\n", tid);
                    //wait() unlocks mutex and then locks again on receiving signal!
                    //this is why the other thread is able to send signal with locks on
                    pthread_cond_wait(&arg->real_lock.fill, &arg->real_lock.lock);
                    printf("consumer#%d...receive 'fill' cond(locked)\n", tid);
                }
            }
            else{
                printf("consumer#%d...skip cond\n", tid);
            }
            printf("get: %d\n", get(&arg->buffer));
            pthread_cond_signal(&arg->real_lock.empty);
            printf("consumer#%d...signal 'empty' cond\n", tid);
            

            pthread_mutex_unlock(&arg->real_lock.lock);
            printf("consumer#%d...unlocked\n", tid);
        }

        printf("ending consumer#%d.\n", tid);
    }

    void test(){
        pthread_t producer, consumer0, consumer1;
        producer = consumer0 = consumer1 = 0;
        arg_t arg = {0};
        

        pthread_create(&producer, NULL, producer_thread, &arg);
        pthread_create(&consumer0, NULL, consumer_thread, &arg);
        pthread_create(&consumer1, NULL, consumer_thread, &arg);
        
        pthread_join(producer, NULL);
        pthread_join(consumer0, NULL);
        pthread_join(consumer1, NULL);
    }

}
