






#ifndef BASE_PROCESS_UTIL_H_
#define BASE_PROCESS_UTIL_H_

#include "base/basictypes.h"

#if defined(OS_WIN)
#include <windows.h>
#include <tlhelp32.h>
#include <io.h>
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#elif defined(OS_LINUX) || defined(__GLIBC__)
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#elif defined(OS_MACOSX)
#include <mach/mach.h>
#endif

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#ifndef OS_WIN
#include <unistd.h>
#endif

#include "base/command_line.h"
#include "base/process.h"

#if defined(OS_WIN)
typedef PROCESSENTRY32 ProcessEntry;
typedef IO_COUNTERS IoCounters;
#elif defined(OS_POSIX)

struct ProcessEntry {
  int pid;
  int ppid;
  char szExeFile[NAME_MAX + 1];
};

struct IoCounters {
  unsigned long long ReadOperationCount;
  unsigned long long WriteOperationCount;
  unsigned long long OtherOperationCount;
  unsigned long long ReadTransferCount;
  unsigned long long WriteTransferCount;
  unsigned long long OtherTransferCount;
};

#include "base/file_descriptor_shuffle.h"
#endif

#if defined(OS_MACOSX)
struct kinfo_proc;
#endif

namespace base {


enum ProcessArchitecture {
  PROCESS_ARCH_I386 = 0x1,
  PROCESS_ARCH_X86_64 = 0x2,
  PROCESS_ARCH_PPC = 0x4,
  PROCESS_ARCH_ARM = 0x8
};

inline ProcessArchitecture GetCurrentProcessArchitecture()
{
  base::ProcessArchitecture currentArchitecture;
#if defined(ARCH_CPU_X86)
  currentArchitecture = base::PROCESS_ARCH_I386;
#elif defined(ARCH_CPU_X86_64)
  currentArchitecture = base::PROCESS_ARCH_X86_64;
#elif defined(ARCH_CPU_PPC)
  currentArchitecture = base::PROCESS_ARCH_PPC;
#elif defined(ARCH_CPU_ARMEL)
  currentArchitecture = base::PROCESS_ARCH_ARM;
#endif
  return currentArchitecture;
}




enum {
  PROCESS_END_NORMAL_TERMINATON = 0,
  PROCESS_END_KILLED_BY_USER    = 1,
  PROCESS_END_PROCESS_WAS_HUNG  = 2
};


ProcessId GetCurrentProcId();


ProcessHandle GetCurrentProcessHandle();



bool OpenProcessHandle(ProcessId pid, ProcessHandle* handle);





bool OpenPrivilegedProcessHandle(ProcessId pid, ProcessHandle* handle);


void CloseProcessHandle(ProcessHandle process);




ProcessId GetProcId(ProcessHandle process);

#if defined(OS_POSIX)





void SetAllFDsToCloseOnExec();



void CloseSuperfluousFds(const base::InjectiveMultimap& saved_map);
#endif

enum ChildPrivileges {
  PRIVILEGES_DEFAULT,
  PRIVILEGES_UNPRIVILEGED,
  PRIVILEGES_INHERIT,
  PRIVILEGES_LAST
};

#if defined(OS_WIN)














bool LaunchApp(const std::wstring& cmdline,
               bool wait, bool start_hidden, ProcessHandle* process_handle);
#elif defined(OS_POSIX)










typedef std::vector<std::pair<int, int> > file_handle_mapping_vector;
bool LaunchApp(const std::vector<std::string>& argv,
               const file_handle_mapping_vector& fds_to_remap,
               bool wait, ProcessHandle* process_handle);

typedef std::map<std::string, std::string> environment_map;
bool LaunchApp(const std::vector<std::string>& argv,
               const file_handle_mapping_vector& fds_to_remap,
               const environment_map& env_vars_to_set,
               ChildPrivileges privs,
               bool wait, ProcessHandle* process_handle,
               ProcessArchitecture arch=GetCurrentProcessArchitecture());
bool LaunchApp(const std::vector<std::string>& argv,
               const file_handle_mapping_vector& fds_to_remap,
               const environment_map& env_vars_to_set,
               bool wait, ProcessHandle* process_handle,
               ProcessArchitecture arch=GetCurrentProcessArchitecture());
#endif



void SetCurrentProcessPrivileges(ChildPrivileges privs);



bool LaunchApp(const CommandLine& cl,
               bool wait, bool start_hidden, ProcessHandle* process_handle);


class ProcessFilter {
 public:
  
  
  virtual bool Includes(ProcessId pid, ProcessId parent_pid) const = 0;
  virtual ~ProcessFilter() { }
};





bool KillProcess(ProcessHandle process, int exit_code, bool wait);








bool DidProcessCrash(bool* child_exited, ProcessHandle handle);





class ProcessMetrics {
 public:
  
  
  static ProcessMetrics* CreateProcessMetrics(ProcessHandle process);

  ~ProcessMetrics();

  
  
  
  
  
  
  int GetCPUUsage();

 private:
  explicit ProcessMetrics(ProcessHandle process);

  ProcessHandle process_;

  int processor_count_;

  
  int64_t last_time_;
  int64_t last_system_time_;

  DISALLOW_EVIL_CONSTRUCTORS(ProcessMetrics);
};

}  

namespace mozilla {

class EnvironmentLog
{
public:
  explicit EnvironmentLog(const char* varname) {
    const char *e = getenv(varname);
    if (e && *e) {
      fname_ = e;
    }
  }

  ~EnvironmentLog() {}

  void print(const char* format, ...) {
    if (!fname_.size())
      return;

    FILE* f;
    if (fname_.compare("-") == 0) {
      f = fdopen(dup(STDOUT_FILENO), "a");
    } else {
      f = fopen(fname_.c_str(), "a");
    }

    if (!f)
      return;

    va_list a;
    va_start(a, format);
    vfprintf(f, format, a);
    va_end(a);
    fclose(f);
  }

private:
  std::string fname_;

  DISALLOW_EVIL_CONSTRUCTORS(EnvironmentLog);
};

} 

#if defined(OS_WIN)

#undef GetMessage
#undef CreateEvent
#undef GetClassName
#undef GetBinaryType
#undef RemoveDirectory
#endif

#endif  
