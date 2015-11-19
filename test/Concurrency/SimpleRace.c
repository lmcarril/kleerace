// RUN: %llvmgcc %s -emit-llvm -O0 -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --libc=uclibc --preempt-after-pthread-success --instrument-all --race-detection=hb %t1.bc
// RUN: test -f %t.klee-out/test000001.race

#include <pthread.h>
#include <klee/klee.h>
int x;
static void *th_task(void * v)
{
    x++;
	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t a;
	pthread_create(&a, NULL, th_task, NULL);	
    x++;
	pthread_join(a, NULL);
	return 0;
}
