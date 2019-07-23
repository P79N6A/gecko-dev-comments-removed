
































#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#pragma GCC optimize ("O0")
void *thread_function(void *data) __attribute__((noinline, optimize("O2")));

void *thread_function(void *data) {
  pid_t thread_id = syscall(SYS_gettid);
  while (true) ;
  asm("");
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
