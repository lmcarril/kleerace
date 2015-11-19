// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime --instrument-all --search=dfs --dpor %t1.bc
// RUN: test -f %t.klee-out/test000002.ktest

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
  klee_thread_preempt(0);
  pthread_mutex_lock(&m);
  x++;
  pthread_mutex_unlock(&m);
  pthread_join(a, NULL);
  return 0;
}
