#pragma once
#include"common.h"
//using a mutex lock and cond to imitate semaphore
namespace zemaphore{
    typedef struct{
        int value;
        pthread_cond_t cond;
        pthread_mutex_t lock;
    } zem_t;

    void zem_init(zem_t* s, int value){
        s->value = value;
        s->cond = {0};
        s->lock = {0};
    }

    void zem_wait(zem_t* s){
        pthread_mutex_lock(&s->lock);
        s->value--;
        while(s->value <= 0){
            pthread_cond_wait(&s->cond, &s->lock);
        }
        
        pthread_mutex_unlock(&s->lock);
    }

    void zem_post(zem_t* s){
        pthread_mutex_lock(&s->lock);
        s->value++;
        pthread_cond_signal(&s->cond);
        pthread_mutex_unlock(&s->lock);
    }
}