// RUN: %llvmgcc %s -emit-llvm -O0 -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --libc=uclibc --preempt-after-pthread-success --instrument-all --race-detection=hb -fork-on-schedule -no-scheduler-bound %t1.bc
// RUN: test -f %t.klee-out/test000001.race
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int count = 0;
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;

void *
inc_count (void* arg)
{
  count++;
  pthread_mutex_lock (&count_mutex);
  pthread_mutex_unlock (&count_mutex);
  return 0;
}

void *
watch_count (void *idp)
{
  pthread_mutex_lock (&count_mutex);
  pthread_mutex_unlock (&count_mutex);
  count++;
  return 0;
}

int
main (int argc, char *argv[])
{
  int i;
  pthread_t a,b;

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init (&count_mutex, NULL);
  pthread_cond_init (&count_threshold_cv, NULL);
  pthread_create (&a, 0, watch_count, 0);
  pthread_create (&b, 0, inc_count, 0);
  

  pthread_join (a, NULL);
  pthread_join (b, NULL);
  pthread_mutex_destroy (&count_mutex);
  return 0;
}
