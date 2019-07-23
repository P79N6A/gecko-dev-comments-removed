






























#include <pthread.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/linux_thread.h"

using namespace google_breakpad;


static bool should_exit = false;

static int foo2(int arg) {
  
  printf("%s:%d\n", __FUNCTION__, __LINE__);
  int c = 0xcccccccc;
  fprintf(stderr, "Thread trying to crash: %x\n", getpid());
  c = *reinterpret_cast<int *>(0x5);
  return c;
}

static int foo(int arg) {
  
  int b = 0xbbbbbbbb;
  b = foo2(b);
  return b;
}

static void *thread_crash(void *) {
  
  int a = 0xaaaaaaaa;
  sleep(1);
  a = foo(a);
  printf("%x\n", a);
  return NULL;
}

static void *thread_main(void *) {
  while (!should_exit)
    sleep(1);
  return NULL;
}

static void CreateCrashThread() {
  pthread_t h;
  pthread_create(&h, NULL, thread_crash, NULL);
  pthread_detach(h);
}


static void CreateThread(int num) {
  pthread_t h;
  for (int i = 0; i < num; ++i) {
    pthread_create(&h, NULL, thread_main, NULL);
    pthread_detach(h);
  }
}


static bool MinidumpCallback(const char *dump_path,
                             const char *minidump_id,
                             void *context,
                             bool succeeded) {
  int index = reinterpret_cast<int>(context);
  printf("%d %s: %s is dumped\n", index, __FUNCTION__, minidump_id);
  if (index == 0) {
    should_exit = true;
    return true;
  }
  
  return false;
}

int main(int argc, char *argv[]) {
  int handler_index = 0;
  ExceptionHandler handler_ignore(".", NULL, MinidumpCallback,
                           (void*)handler_index, true);
  ++handler_index;
  ExceptionHandler handler_process(".", NULL, MinidumpCallback,
                           (void*)handler_index, true);
  CreateCrashThread();
  CreateThread(10);

  while (true)
    sleep(1);
  should_exit = true;

  return 0;
}
