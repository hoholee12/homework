#pragma once
#define _GNU_SOURCE //sched_setaffinity. also needs to be declared on top
#include<stdio.h>
#include<stdlib.h>
#include<string.h> //memset
#include<stddef.h> //NULL def
#include<sys/time.h>
#include<sys/stat.h>
#include<assert.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<signal.h>
#include<poll.h>
#include<sched.h>
#include<sys/random.h>
#include<sys/mman.h>
#include<stdint.h>
#include<sys/syscall.h> //indirect gettid
#include<semaphore.h>
#include<stdarg.h>

#ifdef SYS_gettid
#define gettid() ((pid_t)syscall(SYS_gettid))
#endif



double GetTime(){
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    assert(rc == 0);
    return (double) t.tv_sec + (double)t.tv_usec / 1e6; //10^6(double)

}

void Spin(int howlong){
    double t = GetTime();
    while((GetTime() - t) < (double)howlong);

}

