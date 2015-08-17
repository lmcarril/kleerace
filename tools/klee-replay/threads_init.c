#include "../../runtime/POSIX/threads_init.c"

inline vc_id_t klee_vclock_create() {
  return 0;
}

inline vc_id_t klee_vclock_get(pthread_t tid) {
  return 0;
}

inline void klee_vclock_tock() {
}
