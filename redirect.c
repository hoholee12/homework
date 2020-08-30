#include"common.h"

int main(int argc, char* argv[]){
     printf("pid: %d\n", (int)getpid());
    int rc = fork();
    if(rc == -1) exit(1);
    else if(rc == 0) {
        printf("i am child pid: %d\n", (int)getpid());
        close(STDOUT_FILENO);
        open("./output.txt", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

        char* myargs[3];
        myargs[0] = strdup("wc");
        myargs[1] = strdup(argv[0]);
        myargs[2] = NULL;
        execvp(myargs[0], myargs);  //run wc
    }
    else{ 
        int rc_wait = wait(NULL);
        printf("i am %d and my offspring is %d (wait)%d\n", (int)getpid(), rc, rc_wait);
    }


}