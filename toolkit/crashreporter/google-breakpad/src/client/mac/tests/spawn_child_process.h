































#ifndef GOOGLE_BREAKPAD_CLIENT_MAC_TESTS_SPAWN_CHILD_PROCESS
#define GOOGLE_BREAKPAD_CLIENT_MAC_TESTS_SPAWN_CHILD_PROCESS

#include <AvailabilityMacros.h>
#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif
#include <crt_externs.h>
#include <mach-o/dyld.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
#include <spawn.h>
#endif

#include <string>
#include <vector>

#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad_test {

using std::string;
using std::vector;

const MDCPUArchitecture kNativeArchitecture =
#if defined(__i386__)
  MD_CPU_ARCHITECTURE_X86
#elif defined(__x86_64__)
  MD_CPU_ARCHITECTURE_AMD64
#elif defined(__ppc__) || defined(__ppc64__)
  MD_CPU_ARCHITECTURE_PPC
#else
#error "This file has not been ported to this CPU architecture."
#endif
  ;

const uint32_t kNativeContext =
#if defined(__i386__)
  MD_CONTEXT_X86
#elif defined(__x86_64__)
  MD_CONTEXT_AMD64
#elif defined(__ppc__) || defined(__ppc64__)
  MD_CONTEXT_PPC
#else
#error "This file has not been ported to this CPU architecture."
#endif
  ;

string GetExecutablePath() {
  char self_path[PATH_MAX];
  uint32_t size = sizeof(self_path);
  if (_NSGetExecutablePath(self_path, &size) != 0)
    return "";
  return self_path;
}

string GetHelperPath() {
  string helper_path(GetExecutablePath());
  size_t pos = helper_path.rfind('/');
  if (pos == string::npos)
    return "";

  helper_path.erase(pos + 1);
  helper_path += "minidump_generator_test_helper";
  return helper_path;
}

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6

pid_t spawn_child_process(const char** argv) {
  posix_spawnattr_t spawnattr;
  if (posix_spawnattr_init(&spawnattr) != 0)
    return (pid_t)-1;

  cpu_type_t pref_cpu_types[2] = {
#if defined(__x86_64__)
    CPU_TYPE_X86,
#elif defined(__i386__)
    CPU_TYPE_X86_64,
#endif
    CPU_TYPE_ANY
  };

  
  size_t attr_count = sizeof(pref_cpu_types) / sizeof(pref_cpu_types[0]);
  size_t attr_ocount = 0;
  if (posix_spawnattr_setbinpref_np(&spawnattr,
                                    attr_count,
                                    pref_cpu_types,
                                    &attr_ocount) != 0 ||
      attr_ocount != attr_count) {
    posix_spawnattr_destroy(&spawnattr);
    return (pid_t)-1;
  }

  
  vector<char*> argv_v;
  while (*argv) {
    argv_v.push_back(strdup(*argv));
    argv++;
  }
  argv_v.push_back(NULL);
  pid_t new_pid = 0;
  int result = posix_spawnp(&new_pid, argv_v[0], NULL, &spawnattr,
                            &argv_v[0], *_NSGetEnviron());
  posix_spawnattr_destroy(&spawnattr);
  
  for (unsigned i = 0; i < argv_v.size(); i++) {
    free(argv_v[i]);
  }

  return result == 0 ? new_pid : -1;
}
#endif

}  

#endif  
