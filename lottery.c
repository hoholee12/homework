#include"common.h"

typedef char myint[2];

int main(int argc, char* argv[]){
    
    //code location
    printf("%p\n", main);

    //heap location
    printf("%p\n", malloc(100e6));

    //stack location
    int x = 0;
    printf("%p\n", &x);

    return 0;
}