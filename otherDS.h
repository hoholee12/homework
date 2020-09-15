#pragma once
#include"common.h"
#define LOOPVAL 20000


namespace linkedlist{
    
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
         for(int i = 0; i < LOOPVAL; i++){
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

namespace handoverhandlinkedlist{
    
    typedef struct _node_t{
        int key;
        _node_t* next;
        pthread_mutex_t nlock;
    } node_t;   //fine-grained locking

    typedef struct{
        node_t* head;
    } list_t;
    //init can be done by {0}ing

    typedef struct{
        list_t list;
        int count_correct;
    } arg_t;

    void insert(list_t* list, int key){
        node_t* temp = (node_t*)malloc(sizeof(node_t));
        temp->nlock = PTHREAD_MUTEX_INITIALIZER;
        temp->key = key;
        
        pthread_mutex_t* ltemp = &list->head->nlock;
        
        pthread_mutex_lock(ltemp);    //only required on accessing list(critical section)
        temp->next = list->head;
        list->head = temp;
        pthread_mutex_unlock(ltemp);
    }

    int lookup(list_t* list, int key){
        int result = -1;    //fail

        node_t* temp = list->head;
        pthread_mutex_lock(&temp->nlock);
        while(temp->next){  //prevent lock next if its null
            //success
            if(temp->key == key){
                result = 0;
                break;
            }
            
            //next node
            //backup old node lock
            pthread_mutex_t* ltemp = &temp->nlock;
            temp = temp->next;
            
            //lock first before unlocking old
            //prohibit other thread from modifying the successor
            pthread_mutex_lock(&temp->nlock);
            pthread_mutex_unlock(ltemp);
            
        }

        if(temp->key == key){
            result = 0;
        }
        pthread_mutex_unlock(&temp->nlock);
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
        //first node is assumed to be initialized and in place
        test.list.head = (node_t*)malloc(sizeof(node_t));
        test.list.head->nlock = PTHREAD_MUTEX_INITIALIZER;

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

namespace hashtable{
     typedef struct _node_t{
        int key;
        _node_t* next;
    } node_t;
    typedef struct{
        node_t* head;
        pthread_mutex_t lock;
    } list_t;
    //init can be done by {0}ing

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

    
    typedef struct{
        list_t lists[LOOPVAL];
    } hash_t;

    typedef struct{
        hash_t hash;
        int count_correct;
        pthread_mutex_t flock;
    } arg_t;

    void inserthash(hash_t* hash, int key){
        insert(&hash->lists[key % LOOPVAL], key);
    }

    int lookuphash(hash_t* hash, int key){
        return lookup(&hash->lists[key % LOOPVAL], key);
    }

     void* afunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
        for(int i = 0; i < LOOPVAL; i++){
            inserthash(&carg->hash, i);
            carg->count_correct += -1 * lookuphash(&carg->hash, LOOPVAL - i - 1);
        }
         
    }

    void* bfunc(void* arg){
        arg_t* carg = (arg_t*)arg;
        sched_getaffinity(0, 0, NULL);
         for(int i = 0; i < LOOPVAL; i++){
            inserthash(&carg->hash, i);
            carg->count_correct += -1 * lookuphash(&carg->hash, LOOPVAL - i - 1);
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