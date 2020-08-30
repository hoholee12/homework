#include"common.h"

int main(int argc, char* argv[]){
    printf("pid: %d\n", (int)getpid());
    int rc = fork();
    if(rc == -1) exit(1);
    else if(rc == 0) {
        int rc_wait = wait(NULL);
        printf("i am child pid: %d (wait)%d\n", (int)getpid(), rc_wait);}
    else{ 
        int rc_wait = wait(NULL);
        printf("i am %d and my offspring is %d (wait)%d\n", (int)getpid(), rc, rc_wait);
    }
}