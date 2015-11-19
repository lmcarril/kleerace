// RUN: %llvmgcc%t.klee-out  %s -emit-llvm -O0 -c -g -o %t1.bc
// RUN: %klee --no-output --exit-on-error --posix-runtime --libc=uclibc --preempt-after-pthread-success --instrument-all --race-detection=hb %t1.bc

#include <pthread.h>
#include <klee/klee.h>
int x;
pthread_mutex_t m;
static void *th_task(void * v)
{
    pthread_mutex_lock(&m);
    x++;
    pthread_mutex_unlock(&m);
	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t a;
    pthread_mutex_init(&m,NULL);
	pthread_create(&a, NULL, th_task, NULL);	
    pthread_mutex_lock(&m);
    x++;
    pthread_mutex_unlock(&m);
	pthread_join(a, NULL);
	return 0;
}
