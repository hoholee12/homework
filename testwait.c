#include"common.h"

int main(int argc, char* argv[]){

    pid_t child = fork();

    
    switch(child){
    case -1: break;
    case 0:
        
        close(STDOUT_FILENO) ;  
        printf("child: hello\n");
        
        break;
    default:
       
         printf("parent: goodbye\n");
        break;
    }

    return 0;
}