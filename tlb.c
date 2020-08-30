#include"common.h"

struct myargs{
    long int result_time;
    int size;
    int jump;
};

void* worker(void* arg){
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset); //initialize cpu_set_t datatype
    
    CPU_SET(0, &cpuset); //pin thread to core#0
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    int size = ((struct myargs*)arg)->size;
    int jump = ((struct myargs*)arg)->jump;

    struct timeval start_time, end_time;
    //MAP_PRIVATE = not shareable; page is automatically copied on new write(COW)
    //MAP_ANONYMOUS = mark as not based on anything

    //MAP_POPULATE = prevent demand paging(lazy) by populating on allocation
    int* a = mmap(NULL, size * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS, -1, 0);
    
    gettimeofday(&start_time, NULL);
    for(int i = 0; i < size; i += jump){
        a[i] += 1;
        //printf("jump = %d\n", i >> 10);
    }
    gettimeofday(&end_time, NULL);
    ((struct myargs*)arg)->result_time = end_time.tv_usec - start_time.tv_usec;
    
    munmap(a, size * sizeof(int));

}



int main(int argc, char* argv[]){
    if(argc == 1) return argc;

    struct myargs* hello = malloc(sizeof(struct myargs));
    int numpages = strtol(argv[1], NULL, 10);
    int jumptoggle = strtol(argv[2], NULL, 10);
    printf("numpage = %d\n", numpages);
    int jump = 1024; //sizeof(int) * jump = 4k(single page jump for x86)
    if(jumptoggle != 0) jump = 1;
    int size = numpages * jump;

    hello->size = size;
    hello->jump = jump;
    pthread_t test;
    pthread_create(&test, NULL, worker, (void*)hello);

    pthread_join(test, NULL);

    printf("took %ld microseconds.\n", hello->result_time);

    free(hello);
    return 0;
}