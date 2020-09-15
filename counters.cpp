#include"counters.h"

int main(int argc, char* argv[]){
    simplecounter::test();
    simplecounter_locks::test();
    scalablecounter_approx::test();
    return 0;
}