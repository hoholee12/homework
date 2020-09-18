#include"threadmethods.h"

int main(int argc, char* argv[]){

    simplethreadjoin::test();
    waitforthread::test();
    nolockjoin::test();

    //putandget::test();
    //putandget_twoconsumers::test();
    putandget_mesasemantics::test();
    return 0;
}