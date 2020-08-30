#include"common.h"

int main(int argc, char* argv[]){

    int fds[2];
    pipe(fds);
    struct timeval yourtime;
    struct timeval mytime;
    int i = 0;
    int tempset = 123;
    while(i++ < 1000)
    switch(fork()){
    case -1: return 0;
    case 0:
        
        tempset = sched_getaffinity(0, 0, NULL);
        printf("affinity? %d\t", tempset);
        close(fds[0]);  //close read
        gettimeofday(&mytime, NULL);
        
        write(fds[1], &mytime.tv_usec, sizeof(suseconds_t));
        return 0;
    default:
        tempset = sched_getaffinity(0, 0, NULL);
        printf("affinity? %d\t", tempset);
        close(fds[1]); //close write
        gettimeofday(&mytime, NULL);

        read(fds[0], &yourtime.tv_usec, sizeof(suseconds_t));

        gettimeofday(&yourtime, NULL);
        
        printf("%ld\n", yourtime.tv_usec - mytime.tv_usec);
        
        break;
    }

    return 0;
}