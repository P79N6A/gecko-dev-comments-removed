






























#include <pthread.h>
#include <unistd.h>

#include "client/minidump_file_writer.h"
#include "client/solaris/handler/minidump_generator.h"

using std::string;
using google_breakpad::MinidumpGenerator;

static bool doneWritingReport = false;

static void *Reporter(void *) {
  char buffer[PATH_MAX];
  MinidumpGenerator md;

  
  snprintf(buffer, sizeof(buffer), "./minidump_test.out");
  fprintf(stdout, "Writing %s\n", buffer);

  md.WriteMinidumpToFile(buffer, 0);
  doneWritingReport = true;

  return NULL;
}

static void SleepyFunction() {
  while (!doneWritingReport) {
    usleep(100);
  }
}

int main(int argc, char * const argv[]) {
  pthread_t reporter_thread;

  if (pthread_create(&reporter_thread, NULL, Reporter, NULL) == 0) {
    pthread_detach(reporter_thread);
  } else {
    perror("pthread_create");
  }

  SleepyFunction();

  return 0;
}
