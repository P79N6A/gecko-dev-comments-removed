
































#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "common/scoped_ptr.h"
#include "third_party/lss/linux_syscall_support.h"

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
  int pipefd = *static_cast<int *>(data);
  volatile pid_t thread_id = syscall(__NR_gettid);
  
  uint8_t byte = 1;
  if (write(pipefd, &byte, sizeof(byte)) != sizeof(byte)) {
    perror("ERROR: parent notification failed");
    return NULL;
  }
  register volatile pid_t *thread_id_ptr asm(TID_PTR_REGISTER) = &thread_id;
  while (true)
    asm volatile ("" : : "r" (thread_id_ptr));
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr,
            "usage: linux_dumper_unittest_helper <pipe fd> <# of threads>\n");
    return 1;
  }
  int pipefd = atoi(argv[1]);
  int num_threads = atoi(argv[2]);
  if (num_threads < 1) {
    fprintf(stderr, "ERROR: number of threads is 0");
    return 1;
  }
  google_breakpad::scoped_array<pthread_t> threads(new pthread_t[num_threads]);
  pthread_attr_t thread_attributes;
  pthread_attr_init(&thread_attributes);
  pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
  for (int i = 1; i < num_threads; i++) {
    pthread_create(&threads[i], &thread_attributes, &thread_function, &pipefd);
  }
  thread_function(&pipefd);
  return 0;
}
