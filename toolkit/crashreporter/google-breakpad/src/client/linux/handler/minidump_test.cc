






























#include <pthread.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "client/linux/handler/minidump_generator.h"

using namespace google_breakpad;


static bool should_exit = false;

static void foo2(int arg) {
  
  int c = arg;
  c = 0xcccccccc;
  while (!should_exit)
    sleep(1);
}

static void foo(int arg) {
  
  int b = arg;
  b = 0xbbbbbbbb;
  foo2(b);
}

static void *thread_main(void *) {
  
  int a = 0xaaaaaaaa;
  foo(a);
  return NULL;
}

static void CreateThread(int num) {
  pthread_t h;
  for (int i = 0; i < num; ++i) {
    pthread_create(&h, NULL, thread_main, NULL);
    pthread_detach(h);
  }
}

int main(int argc, char *argv[]) {
  CreateThread(10);
  google_breakpad::MinidumpGenerator mg;
  if (mg.WriteMinidumpToFile("minidump_test.out", -1, NULL))
    printf("Succeeded written minidump\n");
  else
    printf("Failed to write minidump\n");
  should_exit = true;
  return 0;
}
