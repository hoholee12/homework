#include"common.h"


typedef struct{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int myvar;
} thread_args;

void* consumer_thread(void* arg){
    thread_args* myarg = arg;
    
    //use mutex lock to do atomic operation(prevent datarace)
    pthread_mutex_lock(&myarg->lock);
//critical section starts here
    while(myarg->myvar != 1){ //in case of spurious wakeup for cond wait
        printf("waiting...\n");
        pthread_cond_wait(&myarg->cond, &myarg->lock);
    }
    myarg->myvar = 2;
    printf("waiting...finished\n");
//critical section ends here
    pthread_mutex_unlock(&myarg->lock);

    //i can return address of a main stack.
    return &myarg->myvar;
}

void* producer_thread(void* arg){
    thread_args* myarg = arg;
    sleep(1);
    pthread_mutex_lock(&myarg->lock);
    printf("signaling...\n");
    myarg->myvar = 1;
    pthread_cond_signal(&myarg->cond);
    printf("signaling...finished\n");
    pthread_mutex_unlock(&myarg->lock);
}

int main(int argc, char* argv[]){

    pthread_t producer, consumer;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    thread_args hello = {lock, cond, 0};    //coarse grained lock

    pthread_create(&producer, NULL, producer_thread, &hello);
    pthread_create(&consumer, NULL, consumer_thread, &hello);

    int* lol = NULL;
    pthread_join(producer, NULL);
    pthread_join(consumer, &lol);
    //thread_return needs a pointer to a void* to write it

    printf("ready = %d, return = %d\n", hello.myvar, *lol);

    return 0;
}