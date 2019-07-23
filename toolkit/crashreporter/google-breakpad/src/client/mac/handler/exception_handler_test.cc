








































#include <pthread.h>
#include <pwd.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

#include "exception_handler.h"
#include "minidump_generator.h"

using std::string;
using google_breakpad::ExceptionHandler;

static void *SleepyFunction(void *) {
  while (1) {
    sleep(10000);
  }
}

static void Crasher() {
  int *a = NULL;

	fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}

static void SoonToCrash() {
  Crasher();
}

bool MDCallback(const char *dump_dir, const char *file_name,
                void *context, bool success) {
  string path(dump_dir);
  string dest(dump_dir);
  path.append(file_name);
  path.append(".dmp");

  fprintf(stdout, "Minidump: %s\n", path.c_str());
  
  return true;
}

int main(int argc, char * const argv[]) {
  char buffer[PATH_MAX];
  struct passwd *user = getpwuid(getuid());

  
  snprintf(buffer, sizeof(buffer), "/Users/%s/Desktop/", user->pw_name);

  string path(buffer);
  ExceptionHandler eh(path, NULL, MDCallback, NULL, true);
  pthread_t t;

  if (pthread_create(&t, NULL, SleepyFunction, NULL) == 0) {
    pthread_detach(t);
  } else {
    perror("pthread_create");
  }

  
  eh.WriteMinidump();

	
  SoonToCrash();

  return 0;
}
