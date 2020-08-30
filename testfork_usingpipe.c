#include"common.h"

int main(int argc, char* argv[]){

    int fds[2];
    pipe(fds);
    int input = 0;
    int output = 0;
    
    switch(fork()){
    case -1: break;
    case 0:
        
        close(fds[0]);  //close read
        printf("child: hello\n");
        input = 100;
        write(fds[1], &input, sizeof(int));
        break;
    default:
        
        close(fds[1]); //close write
        read(fds[0], &output, sizeof(int));
        
        
        printf("parent: goodbye\n");
        break;
    }

    return 0;
}