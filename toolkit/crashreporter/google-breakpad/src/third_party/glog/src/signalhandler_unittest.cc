

































#include "utilities.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "glog/logging.h"

using namespace GOOGLE_NAMESPACE;

void* DieInThread(void*) {
  
  
  
  
  
  fprintf(stderr, "0x%lx is dying\n", (long)pthread_self());
  
  volatile int a = 0;
  volatile int b = 1 / a;
  fprintf(stderr, "We should have died: b=%d\n", b);
  return NULL;
}

void WriteToStdout(const char* data, int size) {
  if (write(STDOUT_FILENO, data, size) < 0) {
    
  }
}

int main(int argc, char **argv) {
#if defined(HAVE_STACKTRACE) && defined(HAVE_SYMBOLIZE)
  InitGoogleLogging(argv[0]);
#ifdef HAVE_LIB_GFLAGS
  ParseCommandLineFlags(&argc, &argv, true);
#endif
  InstallFailureSignalHandler();
  const std::string command = argc > 1 ? argv[1] : "none";
  if (command == "segv") {
    
    LOG(INFO) << "create the log file";
    LOG(INFO) << "a message before segv";
    
    int *a = (int*)0xDEAD;
    *a = 0;
  } else if (command == "loop") {
    fprintf(stderr, "looping\n");
    while (true);
  } else if (command == "die_in_thread") {
    pthread_t thread;
    pthread_create(&thread, NULL, &DieInThread, NULL);
    pthread_join(thread, NULL);
  } else if (command == "dump_to_stdout") {
    InstallFailureWriter(WriteToStdout);
    abort();
  } else {
    
    puts("OK");
  }
#endif
  return 0;
}
