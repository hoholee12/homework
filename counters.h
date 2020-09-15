#include"common.h"
#define LOOPVAL 1000000

namespace simplecounter{
    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct{
        int value;
    } counter_t;

    typedef struct{
        real_locks_t real_locks;
        counter_t counter;
    } arg_t;


    void increment(counter_t* counter){
        counter->value++;
    }
    void decrement(counter_t* counter){
        counter->value--;
    }
    int get(counter_t* counter){
        return counter->value;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            increment(&carg->counter);
        }
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            decrement(&carg->counter);
        }
        return (void*)&carg->counter;
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
        if(result == &test.counter)
            printf("result = %d, time taken = %dsec\n", test.counter.value, second.tv_sec - first.tv_sec);
    }
}

namespace simplecounter_locks{
    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct{
        int value;
    } counter_t;

    typedef struct{
        real_locks_t real_locks;
        counter_t counter;
    } arg_t;


    void increment(real_locks_t* real_locks, counter_t* counter){
        pthread_mutex_lock(&real_locks->real_lock);
        counter->value++;
        pthread_mutex_unlock(&real_locks->real_lock);
    }
    void decrement(real_locks_t* real_locks, counter_t* counter){
        pthread_mutex_lock(&real_locks->real_lock);
        counter->value--;
        pthread_mutex_unlock(&real_locks->real_lock);
    }
    int get(counter_t* counter){
        return counter->value;
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            increment(&carg->real_locks, &carg->counter);
        }
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            decrement(&carg->real_locks, &carg->counter);
        }
        return (void*)&carg->counter;
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
        if(result == &test.counter)
            printf("result = %d, time taken = %dsec\n", test.counter.value, second.tv_sec - first.tv_sec);
    }
}

namespace scalablecounter_approx{
    const int cores = 4;
    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct{
        //global counter
        int global;
        pthread_mutex_t glock;
        //per core counter
        int local[cores];
        pthread_mutex_t llock[cores];
        //update frequency
        int threshold;
    } counter_t;

    typedef struct{
        real_locks_t real_locks;
        counter_t counter;
    } arg_t;

    void update(counter_t* c, pid_t tid, int localamount){
        int cpu = tid % cores;
        //printf("cpu = %d\t", cpu);
        pthread_mutex_lock(&c->llock[cpu]);
        c->local[cpu] += localamount;
        //printf("local = %d\n", c->local[cpu]);
        //transfer to global
        //cover increment decrement both sides
        if((c->local[cpu] >= c->threshold)
        || (c->local[cpu] <= -1 * c->threshold)){
            pthread_mutex_lock(&c->glock);
            c->global += c->local[cpu];
            pthread_mutex_unlock(&c->glock);
            c->local[cpu] = 0;  //reset to 0
        }
        pthread_mutex_unlock(&c->llock[cpu]);
    }

    int get(counter_t* c){
        pthread_mutex_lock(&c->glock);
        int val = c->global;
        pthread_mutex_unlock(&c->glock);
        return val; //return global counter(approximate)
    }


    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            update(&carg->counter, gettid(), 1);
        }
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            update(&carg->counter, gettid(), -1);
        }
        return (void*)&carg->counter;
    }

     void test(){
        pthread_t a, b;
        arg_t test = {0};
        test.counter.threshold = LOOPVAL / 10;
        struct timeval first;
        struct timeval second;
        gettimeofday(&first, NULL);
        pthread_create(&a, NULL, afunc, &test);
        pthread_create(&b, NULL, bfunc, &test);

        void* result = NULL;
        pthread_join(a, NULL);
        pthread_join(b, &result);

        gettimeofday(&second, NULL);
        if(result == &test.counter)
            printf("result = %d, time taken = %dsec\n", test.counter.global, second.tv_sec - first.tv_sec);
    }
}