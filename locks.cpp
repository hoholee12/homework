#include"common.h"
#include"locks.h"



int main(int argc, char* argv[]){
    simplelock::test();
    dekkerpeterson::test();
    //testandset::test();
    compareandswap::test();
    llsc::test();
    fetchandadd::test();
    //testandset_yield::test();
    
    return 0;
}