#pragma once
#include"common.h"
#define LOOPVAL 20

/*
P(s), sem_wait(s):
    s.value = s.value - 1 
    if (s.value < 0)
        change myself to 'sleep state'
    else
        continue

V(s), sem_post(s):
    s.value = s.value + 1
    if (waiting threads in 'sleep state')
        change random one to 'ready state'    //system gets to decide when to interrupt and run
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

    >if consumer runs first...
consumer...sem(full) wait = 0           (its -1; sleeps)
producer...sem(empty) wait = 10
producer...sem(empty) wait finished = 9
put: 0
producer...sem(full) post = 0
producer...sem(full) post finished = 1  (wakes from sleep)
consumer...sem(full) wait finished = 0  (runs)
get: 0
consumer...sem(empty) post = 9
consumer...sem(empty) post finished = 10
    >and then it stays at...
    empty = 10
    full = 0
    >all the time.


    >if producer runs first...
producer...sem(empty) wait = 10         (its over 0; never sleeps)
producer...sem(empty) wait finished = 9
put: 0
producer...sem(full) post = 0
producer...sem(full) post finished = 1
producer...sem(empty) wait = 9
producer...sem(empty) wait finished = 8
put: 1
producer...sem(full) post = 1
producer...sem(full) post finished = 2
    >producer keeps going until sem(empty) wait goes under 0.(incidentally the end of producer)

*/
//producer/consumer, bounded buffer problem
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

        sem_t printlock;
    } real_lock_t;
    
    typedef struct{
        real_lock_t real_lock;
        buffer_t buffer;
    } arg_t;

    void printsem(const char* str, arg_t* arg, sem_t* sem, ...){
        if(arg != NULL) sem_wait(&arg->real_lock.printlock);
        int semval = 0;
        sem_getvalue(sem, &semval);
        va_list va;
        va_start(va, str);
        vprintf(str, va);
        va_end(va);
        printf(" = %d\n", semval);
        if(arg != NULL) sem_post(&arg->real_lock.printlock);
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
            printsem("producer..sem(empty) wait", arg, &arg->real_lock.empty);
            sem_wait(&arg->real_lock.empty);
            printsem("producer..sem(empty) wait finished", arg, &arg->real_lock.empty);
            
            put(&arg->buffer, i);
            
            printf("put: %d\n", i);
            printsem("producer..sem(full) post", arg, &arg->real_lock.full);
            sem_post(&arg->real_lock.full);
            printsem("producer..sem(full) post finished", arg, &arg->real_lock.full);
            //sched_yield();  //to get it evenly running for the test
        }
        printf("producer finished.\n");
        printsem("empty", arg, &arg->real_lock.empty);
        printsem("full", arg, &arg->real_lock.full);
    }

    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        int temp = 0;
        while(temp != LOOPVAL - 1){ //producer ends at LOOPVAL - 1
            printsem("consumer...sem(full) wait", arg, &arg->real_lock.full);
            sem_wait(&arg->real_lock.full);
            printsem("consumer...sem(full) wait finished", arg, &arg->real_lock.full);

            temp = get(&arg->buffer);
            
            printf("get: %d\n", temp);
            printsem("consumer...sem(empty) post", arg, &arg->real_lock.empty);
            sem_post(&arg->real_lock.empty);
            printsem("consumer...sem(empty) post finished", arg, &arg->real_lock.empty);
        }
        printf("consumer finished.\n");
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.real_lock.empty, 0, MAX);
        sem_init(&arg.real_lock.full, 0, 0);
        //FOR PRINT
        sem_init(&arg.real_lock.printlock, 0, 1);

        printsem("empty", 0, &arg.real_lock.empty);
        printsem("full", 0, &arg.real_lock.full);

        pthread_t producer_a = 0, producer_b = 0, consumer = 0;
        printf("begin\n");
        pthread_create(&producer_a, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);

        pthread_join(producer_a, NULL);
        pthread_join(consumer, NULL);
        printf("end\n");

        printsem("empty", 0, &arg.real_lock.empty);
        printsem("full", 0, &arg.real_lock.full);
        
    }

}

//above example ignores mutual exclusion
//lock to prevent data race
//"binary semaphore": very similar to mutex lock
/*
>if consumer runs first:
    >if two producers, one consumer:
consumer...sem(full) wait = 0
producer...sem(empty) wait = 10
producer...sem(empty) wait finished = 9     (interrupt by system)
producer...sem(empty) wait = 9
producer...sem(empty) wait finished = 8     (interrupt by system)
put: 0                                      (interrupt by system)
put: 0                                      (interrupt by system)
producer...sem(full) post = 0
producer...sem(full) post finished = 1
consumer...sem(full) wait finished = 0
get: 0
consumer...sem(empty) post = 8
producer...sem(full) post = 0
producer...sem(full) post finished = 1

    >if mutex lock applied...
consumer...sem(full) wait = 0
producer...sem(empty) wait = 10
producer...sem(empty) wait finished = 9
producer...lock = 1
producer...lock = 0
producer...lock = 1
put: 0
producer...sem(full) post = 0
producer...sem(full) post finished = 1
consumer...sem(full) wait finished = 0
consumer...lock = 1
consumer...lock = 0
consumer...lock = 1
get: 0
consumer...sem(empty) post = 9
consumer...sem(empty) post finished = 10
consumer...sem(full) wait = 0
producer...sem(empty) wait = 10
producer...sem(empty) wait finished = 9
producer...lock = 1
producer...lock = 0
producer...lock = 1
put: 0
producer...sem(full) post = 0
producer...sem(full) post finished = 1
*/
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
        //pthread_mutex_t lock;

        sem_t printlock;
    } real_lock_t;
    
    typedef struct{
        real_lock_t real_lock;
        buffer_t buffer;
    } arg_t;

    void printsem(const char* str, arg_t* arg, sem_t* sem, ...){
        if(arg != NULL) sem_wait(&arg->real_lock.printlock);
        int semval = 0;
        sem_getvalue(sem, &semval);
        va_list va;
        va_start(va, str);
        vprintf(str, va);
        va_end(va);
        printf(" = %d\n", semval);
        if(arg != NULL) sem_post(&arg->real_lock.printlock);
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
        pid_t pid = gettid() % 2;
        sleep(1);   //to get it evenly running for the test
        for(int i = 0; i < LOOPVAL; i++){
            printsem("producer#%d...sem(empty) wait", arg, &arg->real_lock.empty, pid);
            sem_wait(&arg->real_lock.empty);
            printsem("producer#%d...sem(empty) wait finished", arg, &arg->real_lock.empty, pid);
            printsem("producer#%d...lock", arg, &arg->real_lock.lock, pid);
            sem_wait(&arg->real_lock.lock);
            printsem("producer#%d...lock", arg, &arg->real_lock.lock, pid);
            //pthread_mutex_lock(&arg->real_lock.lock);
            put(&arg->buffer, i);
            //pthread_mutex_unlock(&arg->real_lock.lock);
            sem_post(&arg->real_lock.lock);
            printsem("producer#%d...lock", arg, &arg->real_lock.lock, pid);
            printf("put: %d\n", i);
            printsem("producer#%d...sem(full) post", arg, &arg->real_lock.full, pid);
            sem_post(&arg->real_lock.full);
            printsem("producer#%d...sem(full) post finished", arg, &arg->real_lock.full, pid);
            sched_yield();  //to get it evenly running for the test
        }
        printf("producer#%d finished.\n", pid);
        printsem("empty", arg, &arg->real_lock.empty);
        printsem("full", arg, &arg->real_lock.full);
    }

    void* consumer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        int temp = 0;
        //sleep(1);
        while(temp != LOOPVAL - 1){ //producer ends at LOOPVAL - 1
            //sched_yield();
            printsem("consumer...sem(full) wait", arg, &arg->real_lock.full);
            sem_wait(&arg->real_lock.full);
            printsem("consumer...sem(full) wait finished", arg, &arg->real_lock.full);
            printsem("consumer...lock", arg, &arg->real_lock.lock);
            sem_wait(&arg->real_lock.lock);
            printsem("consumer...lock", arg, &arg->real_lock.lock);
            //pthread_mutex_lock(&arg->real_lock.lock);
            temp = get(&arg->buffer);
            //pthread_mutex_unlock(&arg->real_lock.lock);
            sem_post(&arg->real_lock.lock);
            printsem("consumer...lock", arg, &arg->real_lock.lock);
            printf("get: %d\n", temp);
            printsem("consumer...sem(empty) post", arg, &arg->real_lock.empty);
            sem_post(&arg->real_lock.empty);
            printsem("consumer...sem(empty) post finished", arg, &arg->real_lock.empty);
        }
        printf("consumer finished.\n");
    }

    void test(){
        arg_t arg = {0};
        sem_init(&arg.real_lock.empty, 0, MAX);
        sem_init(&arg.real_lock.full, 0, 0);
        sem_init(&arg.real_lock.lock, 0, 1);    //1 required as initial value for binary sem

        sem_init(&arg.real_lock.printlock, 0, 1);

        printsem("empty", 0, &arg.real_lock.empty);
        printsem("full", 0, &arg.real_lock.full);
        printsem("lock", 0, &arg.real_lock.lock);

        pthread_t producer_a = 0, producer_b = 0, consumer = 0;
        printf("begin\n");
        pthread_create(&producer_a, NULL, producer_thread, &arg);
        pthread_create(&producer_b, NULL, producer_thread, &arg);
        pthread_create(&consumer, NULL, consumer_thread, &arg);

        pthread_join(producer_a, NULL);
        pthread_join(producer_b, NULL);
        pthread_join(consumer, NULL);
        printf("end\n");

        printsem("empty", 0, &arg.real_lock.empty);
        printsem("full", 0, &arg.real_lock.full);
        printsem("lock", 0, &arg.real_lock.lock);
        
    }

}

namespace simple_readwritelock{

    typedef struct{
        sem_t lock;         //binary semaphore
        sem_t writelock;    //for writer:reader = 1:n ratio
        int readers;        //critical section
    } rwlock_t;

#define MAX 10
    typedef struct{
        int arr[MAX];
        int fill_index;
        int use_index;
        int count;
    } buffer_t;
    
    typedef struct{
        rwlock_t rwlock;
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

    void rwlock_init(rwlock_t* rwlock){
        rwlock->readers = 0;
        //binary semaphores: init to 1
        sem_init(&rwlock->lock, 0, 1);
        sem_init(&rwlock->writelock, 0, 1);
    }

    void rwlock_acquire_readlock(rwlock_t* rwlock){
        sem_wait(&rwlock->lock);
        rwlock->readers++;
        if(rwlock->readers == 1)
            sem_wait(&rwlock->writelock);
        sem_post(&rwlock->lock);
    }

    void rwlock_release_readlock(rwlock_t* rwlock){
        sem_wait(&rwlock->lock);
        rwlock->readers--;
        if(rwlock->readers == 0)
            sem_wait(&rwlock->writelock);
        sem_post(&rwlock->lock);
    }

    void rwlock_acquire_writelock(rwlock_t* rwlock){
        sem_wait(&rwlock->writelock);
    }
    
    void rwlock_release_writelock(rwlock_t* rwlock){
        sem_post(&rwlock->writelock);
    }

    void* writer_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        for(int i = 0; i < LOOPVAL; i++){
            printf("writer 0\n");
            rwlock_acquire_readlock(&arg->rwlock);
            printf("writer 1\n");
            for(int j = 0; j < MAX; j++){
                put(&arg->buffer, i * j);
            }
            printf("writer 2\n");
            rwlock_release_readlock(&arg->rwlock);
            printf("writer 3\n");
        }
    }

    void* reader_thread(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;
        pid_t pid = gettid() % MAX;
        int temp = 0;
        while(temp < (LOOPVAL - 2) * MAX){
            rwlock_acquire_writelock(&arg->rwlock);
            temp = get(&arg->buffer);
            printf("%d\n", temp);
            rwlock_release_writelock(&arg->rwlock);
        }
    }

    void test(){
        arg_t arg = {0};
        rwlock_init(&arg.rwlock);
        
        pthread_t writer = 0;
        pthread_t reader[MAX] = {0};

        printf("begin\n");
        pthread_create(&writer, NULL, writer_thread, &arg);
        for(int i = 0; i < MAX; i++)
            pthread_create(&reader[i], NULL, reader_thread, &arg);

        pthread_join(writer, NULL);
        for(int i = 0; i < MAX; i++)
            pthread_join(reader[i], NULL);
        
        printf("end\n");

    }

}

//dining philosophers problem(round table)
/*
thread#8 thinks...
philosopher#8 = 1
wait myself: philosopher#8 = 0
wait right: philosopher#9 = 0
thread#8 eats...
philosopher#8 ends.
thread#9 thinks...
>>>>>>>>>>>>>>>>>switched philosopher#9 = 1
wait right: philosopher#0 = 0
wait myself: philosopher#9 = 0
thread#9 eats...
*/
namespace diningtable{
#define MAX 10

    sem_t forks[MAX];
    sem_t printlock;

    typedef struct{
        int num_loops;
        int thread_id;
    } arg_t;

    void printsem(const char* str, sem_t* sem, ...){
        sem_wait(&printlock);
        int semval = 0;
        sem_getvalue(sem, &semval);
        va_list va;
        va_start(va, str);
        vprintf(str, va);
        va_end(va);
        printf(" = %d\n", semval);
        sem_post(&printlock);
    }

    int left(int p){
        return p;
    }
    int right(int p){
        return (p + 1) % MAX;
    }

    void get_forks(int p){
        //break dependency on last philosopher by looking at right first
        //solve deadlock
        
        if(p == MAX - 1){
            printf(">>>>>>>>>>>>>>>>>switched ");
            printsem("philosopher#%d", &forks[p], p);
            sem_wait(&forks[right(p)]);
            printsem("wait right: philosopher#%d", &forks[right(p)], right(p));
            sem_wait(&forks[left(p)]);
            printsem("wait myself: philosopher#%d", &forks[left(p)], left(p));
        }
        else{
            printsem("philosopher#%d", &forks[p], p);
            sem_wait(&forks[left(p)]);
            printsem("wait myself: philosopher#%d", &forks[left(p)], left(p));
            sem_wait(&forks[right(p)]);
            printsem("wait right: philosopher#%d", &forks[right(p)], right(p));
        }
    }

    void put_forks(int p){
        sem_post(&forks[left(p)]);
        sem_post(&forks[right(p)]);
    }

    void think(int p){ printf("thread#%d thinks...\n", p); return; }
    void eat(int p){ printf("thread#%d eats...\n", p); return; }

    void* philosopher(void* arg_tmp){
        arg_t* arg = (arg_t*)arg_tmp;

        int p = arg->thread_id;

        for(int i = 0; i < arg->num_loops; i++){
            think(p);
            get_forks(p);
            eat(p);
            put_forks(p);
        }
        printf("philosopher#%d ends.\n", p);
    }

    void test(){
        sem_init(&printlock, 0, 1);

        for(int i = 0; i < MAX; i++){
            //init binary sems
            sem_init(&forks[i], 0, 1);
        }

        pthread_t p[MAX];
        arg_t arg[MAX];
        for(int i = 0; i < MAX; i++){
            //init threads
            arg[i].num_loops = LOOPVAL;
            arg[i].thread_id = i;
            pthread_create(&p[i], NULL, philosopher, &arg[i]);
        }

        for(int i = 0; i < MAX; i++){
            pthread_join(p[i], NULL);
        }

    }
}