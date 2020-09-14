#pragma once
#include"common.h"
#define LOOPVAL 10000000

//super skippy
namespace simplelock{
    typedef struct{
        int flag;
    } lock_t;

    typedef struct{
        lock_t lock;
        int testvalue;
    } arg_t;

    void lock(lock_t* mutex){
        //1: using, 0: release
        //another thread is using critical section
        while(mutex->flag == 1);   //spin
        mutex->flag = 1;    //after 0, i get to use it
    }

    void unlock(lock_t* mutex){
        mutex->flag = 0;    //release
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
        return &carg->testvalue;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }
}

//less skippy
namespace dekkerpeterson{
    typedef struct{
        int flag[2];
        int turn;
    } lock_t;

    typedef struct{
        lock_t lock;
        int testvalue;
    } arg_t;

    //assuming only two threads 0 and 1
    void lock(lock_t* mutex, int self){
        //if flag = 0
        mutex->flag[self] = 1;  //flag[0]=1
        int next = 1 - self;    //0->1
        mutex->turn = next;

        //wait until other thread unlocks and sets turn to mine
        while((mutex->flag[next] == 1) && (mutex->turn == next));   //spin
    }

    void unlock(lock_t* mutex, int self){
        mutex->flag[self] = 0;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->lock, 0);
            carg->testvalue++;
            unlock(&carg->lock, 0);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->lock, 1);
            carg->testvalue++;
            unlock(&carg->lock, 1);
        }
        return &carg->testvalue;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }
}

//fails on higher iteration
namespace testandset{

    typedef struct{
        int flag;
    } lock_t;

    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        
    }real_locks_t;

    typedef struct{
        real_locks_t real_lock;
        lock_t lock;
        int testvalue;
    } arg_t;

    //atomic testandset
    int TestAndSet(real_locks_t* real_locks, int* ptr, int val){
        pthread_mutex_lock(&real_locks->real_lock);
        int old = *ptr;
        *ptr = val;
        pthread_mutex_unlock(&real_locks->real_lock);
        return old;
    }

    
    void lock(real_locks_t* real_locks, lock_t* mutex){
        //try to set 1(lock) until old value turns to 0(other thread unlocks)
        while(TestAndSet(real_locks, &mutex->flag, 1) == 1);   //spin
    }

    void unlock(lock_t* mutex){
        mutex->flag = 0;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
        return &carg->testvalue;
    }

     void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }
}

//perfect
namespace compareandswap{

    typedef struct{
        int flag;
    } lock_t;

    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        
    }real_locks_t;

    typedef struct{
        real_locks_t real_lock;
        lock_t lock;
        int testvalue;
    } arg_t;

    //atomic compareandswap
    int CompareAndSwap(real_locks_t* real_locks, int* ptr, int expected, int val){
        pthread_mutex_lock(&real_locks->real_lock);
        int old = *ptr;
        if (old == expected) //it will not necessarily try to set 1 everytime unlike testandset
            *ptr = val;
        pthread_mutex_unlock(&real_locks->real_lock);
        return old;
    }

    
    void lock(real_locks_t* real_locks, lock_t* mutex){
        //try to set 1(lock) until old value turns to 0(other thread unlocks)
        while(CompareAndSwap(real_locks, &mutex->flag, 0, 1) == 1);   //spin
    }

    void unlock(lock_t* mutex){
        mutex->flag = 0;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
        return &carg->testvalue;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }

}

//perfect but slower
namespace llsc{

    typedef struct{
        int flag;
    } lock_t;

    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct{
        real_locks_t real_lock;
        lock_t lock;
        int testvalue;
    } arg_t;

    //atomic ll
    int LoadLinked(real_locks_t* real_locks, int* ptr){
        pthread_mutex_lock(&real_locks->real_lock);
        real_locks->old = *ptr;
        pthread_mutex_unlock(&real_locks->real_lock);
        return real_locks->old;    //get value of pointed flag
    }
    //atomic sc
    int StoreConditional(real_locks_t* real_locks, int* ptr, int val){
        pthread_mutex_lock(&real_locks->real_lock);
        //if no update to pointed flag since ll'd
        if(*ptr == real_locks->old){
            *ptr = val;
            pthread_mutex_unlock(&real_locks->real_lock);
            return 1;   //success
        }
        pthread_mutex_unlock(&real_locks->real_lock);
        return 0;
    }


    
    void lock(real_locks_t* real_locks, lock_t* mutex){
       while(1){
            while(LoadLinked(real_locks, &mutex->flag) == 1);   //spin
            if(StoreConditional(real_locks, &mutex->flag, 1) == 1)
                return; //success
            //otherwise try again
       }
    }

    void unlock(lock_t* mutex){
        mutex->flag = 0;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
        return &carg->testvalue;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }
}

//perfect but even slower
namespace fetchandadd{
    //ticket lock
    typedef struct{
        int ticket;
        int turn;
    } lock_t;

    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct{
        real_locks_t real_lock;
        lock_t lock;
        int testvalue;
    } arg_t;

    //atomic fetchandadd
    int FetchAndAdd(real_locks_t* real_locks, int* ptr){
        pthread_mutex_lock(&real_locks->real_lock);
        int old = *ptr;
        *ptr = old + 1; //increment
        pthread_mutex_unlock(&real_locks->real_lock);
        return old;
    }

    
    void lock(real_locks_t* real_locks, lock_t* mutex){
        //increment ticket
        //and spin until old ticket's turn:
        //ticket - turn = 1
        int myturn = FetchAndAdd(real_locks, &mutex->ticket);
        //ticket - turn = 2
        while(mutex->turn != myturn);    //spin
        //ticket - turn = 1
    }

    void unlock(lock_t* mutex){
        //increment turn
        mutex->turn = mutex->turn + 1;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
        return &carg->testvalue;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }
}

//fails on higher iteration
namespace testandset_yield{

    typedef struct{
        int flag;
    } lock_t;

    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        
    }real_locks_t;

    typedef struct{
        real_locks_t real_lock;
        lock_t lock;
        int testvalue;
    } arg_t;

    //atomic testandset
    int TestAndSet(real_locks_t* real_locks, int* ptr, int val){
        pthread_mutex_lock(&real_locks->real_lock);
        int old = *ptr;
        *ptr = val;
        pthread_mutex_unlock(&real_locks->real_lock);
        return old;
    }

    
    void lock(real_locks_t* real_locks, lock_t* mutex){
        //try to set 1(lock) until old value turns to 0(other thread unlocks)
        while(TestAndSet(real_locks, &mutex->flag, 1) == 1)
            pthread_yield();   //yield
    }

    void unlock(lock_t* mutex){
        mutex->flag = 0;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            lock(&carg->real_lock, &carg->lock);
            carg->testvalue++;
            unlock(&carg->lock);
        }
        return &carg->testvalue;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.testvalue)
            printf("result = %d, time taken = %dsec\n", test.testvalue, second.tv_sec - first.tv_sec);
    }
}
