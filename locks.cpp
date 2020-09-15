#include"common.h"
#include"locks.h"



int main(int argc, char* argv[]){
    simplelock::test();
    dekkerpeterson::test();
    //testandset::test();
    compareandswap::test();
    //testandset_yield::test();
    compareandswap_yield::test();
    llsc::test();
    fetchandadd::test();
    
    
    return 0;
}