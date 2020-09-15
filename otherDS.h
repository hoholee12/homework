#pragma once
#include"common.h"
#define LOOPVAL 50000


namespace simplelinkedlist{
    
    typedef struct _node_t{
        int key;
        _node_t* next;
    } node_t;
    typedef struct{
        node_t* head;
        pthread_mutex_t lock;
    } list_t;
    //init can be done by {0}ing
    
    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct{
        real_locks_t real_locks;
        list_t list;
        int count_correct;
    } arg_t;

    void insert(list_t* list, int key){
        node_t* temp = (node_t*)malloc(sizeof(node_t));
        temp->key = key;
        pthread_mutex_lock(&list->lock);    //only required on accessing list(critical section)
        temp->next = list->head;
        list->head = temp;
        pthread_mutex_unlock(&list->lock);
    }

    int lookup(list_t* list, int key){
        int result = -1;    //fail
        
        pthread_mutex_lock(&list->lock);
        node_t* temp = list->head;
        while(temp){
            //success
            if(temp->key == key){
                result = 0;
                break;
            }

            //next node
            temp = temp->next;
        }

        pthread_mutex_unlock(&list->lock);
        return result;
    }

     void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            insert(&carg->list, i);
            carg->count_correct += -1 * lookup(&carg->list, LOOPVAL - i - 1);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = LOOPVAL - 1; i >= 0; i--){
            insert(&carg->list, i);
            carg->count_correct += -1 * lookup(&carg->list, LOOPVAL - i - 1);
        }
        return &carg->count_correct;
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
        if(result == &test.count_correct)
            printf("result = %d, time taken = %dsec\n", test.count_correct, second.tv_sec - first.tv_sec);
    }

}
