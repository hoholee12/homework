#include"counters.h"

int main(int argc, char* argv[]){
    sloppycounter::test();
    lockedcounter::test();
    scalablecounter_approx::test();
    return 0;
}