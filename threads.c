#include"common.h"


typedef struct{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int myvar;
} thread_args;

void* looper_thread(void* arg){
    thread_args* myarg = arg;
    
    pthread_mutex_lock(&myarg->lock);
    printf("waiting...\n");
    while(myarg->myvar != 1) pthread_cond_wait(&myarg->cond, &myarg->lock);
    myarg->myvar = 2;
    printf("waiting...finished\n");
    pthread_mutex_unlock(&myarg->lock);
}

void* signalmaker_thread(void* arg){
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

    pthread_t signalmaker, looper;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    thread_args hello = {lock, cond, 0};

    pthread_create(&signalmaker, NULL, signalmaker_thread, &hello);
    pthread_create(&looper, NULL, looper_thread, &hello);

    pthread_join(signalmaker, NULL);
    pthread_join(looper, NULL);

    printf("ready = %d\n", hello.myvar);

    return 0;
}