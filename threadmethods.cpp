#include"threadmethods.h"

int main(int argc, char* argv[]){

    simplethreadjoin::test();
    waitforthread::test();
    nolockjoin::test();

    putandget::test();
    return 0;
}