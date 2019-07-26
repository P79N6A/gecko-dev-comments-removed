
































#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__ARM_EABI__)
#define TID_PTR_REGISTER "r3"
#elif defined(__i386)
#define TID_PTR_REGISTER "ecx"
#elif defined(__x86_64)
#define TID_PTR_REGISTER "rcx"
#else
#error This test has not been ported to this platform.
#endif

void *thread_function(void *data) {
  volatile pid_t thread_id = syscall(SYS_gettid);
  register volatile pid_t *thread_id_ptr asm(TID_PTR_REGISTER) = &thread_id;
  while (true)
    asm volatile ("" : : "r" (thread_id_ptr));
  return NULL;
}

int main(int argc, char *argv[]) {
  int num_threads = atoi(argv[1]);
  if (num_threads < 1) {
    fprintf(stderr, "ERROR: number of threads is 0");
    return 1;
  }
  pthread_t threads[num_threads];
  pthread_attr_t thread_attributes;
  pthread_attr_init(&thread_attributes);
  pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
  for (int i = 1; i < num_threads; i++) {
    pthread_create(&threads[i], &thread_attributes, &thread_function, NULL);
  }
  thread_function(NULL);
  return 0;
}
