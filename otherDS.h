#pragma once
#include"common.h"
#define LOOPVAL 10000


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
        list_t list;
        int count_correct;
        pthread_mutex_t flock;
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
            pthread_mutex_lock(&carg->flock);
            insert(&carg->list, i);
            carg->count_correct += -1 * lookup(&carg->list, LOOPVAL - i - 1);
            pthread_mutex_unlock(&carg->flock);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            pthread_mutex_lock(&carg->flock);
            insert(&carg->list, i);
            carg->count_correct += -1 * lookup(&carg->list, LOOPVAL - i - 1);
            pthread_mutex_unlock(&carg->flock);
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

//fucking deadlock
namespace scalablelinkedlist{

    typedef struct{
        pthread_mutex_t real_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t real_cond = PTHREAD_COND_INITIALIZER;
        int old;
    } real_locks_t;

    typedef struct _node_t{
        int key;
        real_locks_t real_locks;    //each element has its own lock
        struct _node_t* next;
    } node_t;
    typedef struct{
        node_t* head;
        pthread_mutex_t lock;   //one for header
    } list_t;

    typedef struct{
        list_t list;
        int count_correct;
    } arg_t;

    typedef struct{
        node_t nodearr[LOOPVAL];
    } freespace_t;

    int checkempty(node_t* node){
        printf("tid = %d, key = %d, next = %08X\n", gettid(), node->key, node->next);
        fflush(stdout);
        if(node->key) return 0;
        printf("tid = %d, checkempty 1\n", gettid());
        fflush(stdout);
        if(node->next) return 0;
        printf("tid = %d, checkempty 2\n", gettid());
        fflush(stdout);
        return 1;   //empty
    }

    node_t* nodealloc(list_t* list){
        node_t* temp = 0;
        printf("tid = %d, nodealloc 1\n", gettid());
        fflush(stdout);
        printf("tid = %d, nodealloc 2\n", gettid());
        fflush(stdout);
        for(int i = 0; i < LOOPVAL; i++){
            temp = list->head;
            printf("tid = %d, nodealloc 3\n", gettid());
            fflush(stdout);
            if(checkempty(temp)){
                printf("tid = %d, nodealloc 4\n", gettid());
                fflush(stdout);
                temp->next = temp + sizeof(node_t);
                break;
            }
        }
        printf("tid = %d, nodealloc 5\n", gettid());
        fflush(stdout);
        return temp;
    }

    void insert(list_t* list, int key){
        pthread_mutex_lock(&list->lock);    //only required on accessing list(critical section)
        node_t* temp = nodealloc(list);
        temp->key = key;
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
         for(int i = 0; i < LOOPVAL; i++){
            insert(&carg->list, i);
            carg->count_correct += -1 * lookup(&carg->list, LOOPVAL - i - 1);
        }
        return &carg->count_correct;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        freespace_t freespace = {0};
        test.list.head = freespace.nodearr;
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

namespace michaelscottqueue{
    typedef struct _node_t{
        int value;
        struct _node_t* next;
    } node_t;

    typedef struct _queue_t{
        node_t* head;
        node_t* tail;
        pthread_mutex_t head_lock;
        pthread_mutex_t tail_lock;
    } queue_t;

    typedef struct{
        queue_t queue;
        int count_correct;
        pthread_mutex_t flock;
        pthread_cond_t fcond;
        int flag;
    } arg_t;

    void initqueue(queue_t* queue){
        node_t* temp = (node_t*)malloc(sizeof(node_t));
        temp->next = NULL;
        queue->head = queue->tail = temp;
    }

    void enqueue(queue_t* queue, int value){
        node_t* temp = (node_t*)malloc(sizeof(node_t));
        temp->value = value;
        temp->next = NULL;

        pthread_mutex_lock(&queue->tail_lock);
        queue->tail->next = temp;
        queue->tail = temp;
        pthread_mutex_unlock(&queue->tail_lock);

    }

    int dequeue(queue_t* queue, int* result){
        pthread_mutex_lock(&queue->head_lock);
        node_t* temp = queue->head;
        node_t* new_head = temp->next;
        if(new_head == NULL){
            pthread_mutex_unlock(&queue->head_lock);
            return -1;
        }
        *result = new_head->value;
        queue->head = new_head;
        pthread_mutex_unlock(&queue->head_lock);
        free(temp);
        return 0;
    }

    void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            enqueue(&carg->queue, i);
        }
        
        //send signal
        carg->flag = 1;
        pthread_cond_signal(&carg->fcond);
        
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            enqueue(&carg->queue, i);
            
        }
        
        //check if finished
        while(!carg->flag) pthread_cond_wait(&carg->fcond, &carg->flock);
        
        //check
        int arr[LOOPVAL] = {0};
        int temp = 0;
        for(int i = 0; i < LOOPVAL; i++){
            if(dequeue(&carg->queue, &temp) == 0)
                arr[temp]++;
        }
        for(int i = 0; i < LOOPVAL; i++){
            if(arr[i] > 1) carg->count_correct++;
        }
        
        return &carg->count_correct;
    }

    void test(){
        pthread_t a, b;
        arg_t test = {0};
        initqueue(&test.queue);
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