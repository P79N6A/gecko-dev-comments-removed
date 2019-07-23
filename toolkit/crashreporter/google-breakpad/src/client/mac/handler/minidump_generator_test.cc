




























#include <unistd.h>

#include <pthread.h>
#include <pwd.h>

#include <CoreFoundation/CoreFoundation.h>

#include "minidump_generator.h"
#include "minidump_file_writer.h"

using std::string;
using google_breakpad::MinidumpGenerator;

static bool doneWritingReport = false;

static void *Reporter(void *) {
  char buffer[PATH_MAX];
  MinidumpGenerator md;
  struct passwd *user = getpwuid(getuid());

  
  snprintf(buffer,
           sizeof(buffer),
           "/Users/%s/Desktop/test.dmp",
           user->pw_name);
  
  fprintf(stdout, "Writing %s\n", buffer);
  unlink(buffer);
  md.Write(buffer);
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
