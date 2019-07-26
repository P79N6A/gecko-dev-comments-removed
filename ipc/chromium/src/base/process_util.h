






#ifndef BASE_PROCESS_UTIL_H_
#define BASE_PROCESS_UTIL_H_

#include "base/basictypes.h"

#if defined(OS_WIN)
#include <windows.h>
#include <tlhelp32.h>
#elif defined(OS_LINUX)
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#elif defined(OS_MACOSX)
#include <mach/mach.h>
#endif

#include <map>
#include <string>
#include <vector>

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
  PRIVILEGES_CAMERA,
  PRIVILEGES_VIDEO,
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



void SetCurrentProcessPrivileges(ChildPrivileges privs);

#endif



bool LaunchApp(const CommandLine& cl,
               bool wait, bool start_hidden, ProcessHandle* process_handle);

#if defined(OS_WIN)




bool GetAppOutput(const std::wstring& cmd_line, std::string* output);
#elif defined(OS_POSIX)





bool GetAppOutput(const CommandLine& cl, std::string* output);
#endif


class ProcessFilter {
 public:
  
  
  virtual bool Includes(ProcessId pid, ProcessId parent_pid) const = 0;
  virtual ~ProcessFilter() { }
};




int GetProcessCount(const std::wstring& executable_name,
                    const ProcessFilter* filter);






bool KillProcesses(const std::wstring& executable_name, int exit_code,
                   const ProcessFilter* filter);





bool KillProcess(ProcessHandle process, int exit_code, bool wait);
#if defined(OS_WIN)
bool KillProcessById(ProcessId process_id, int exit_code, bool wait);
#endif








bool DidProcessCrash(bool* child_exited, ProcessHandle handle);





bool WaitForExitCode(ProcessHandle handle, int* exit_code);





bool WaitForProcessesToExit(const std::wstring& executable_name,
                            int wait_milliseconds,
                            const ProcessFilter* filter);



bool WaitForSingleProcess(ProcessHandle handle,
                          int wait_milliseconds);



bool CrashAwareSleep(ProcessHandle handle, int wait_milliseconds);







bool CleanupProcesses(const std::wstring& executable_name,
                      int wait_milliseconds,
                      int exit_code,
                      const ProcessFilter* filter);





class NamedProcessIterator {
 public:
  NamedProcessIterator(const std::wstring& executable_name,
                       const ProcessFilter* filter);
  ~NamedProcessIterator();

  
  
  
  
  
  const ProcessEntry* NextProcessEntry();

 private:
#if !defined(OS_BSD)
  
  
  
  bool CheckForNextProcess();

  bool IncludeEntry();

  
  
  void InitProcessEntry(ProcessEntry* entry);

  std::wstring executable_name_;
#endif

#if defined(OS_WIN)
  HANDLE snapshot_;
  bool started_iteration_;
#elif defined(OS_LINUX)
  DIR *procfs_dir_;
#elif defined(OS_BSD)
  std::vector<ProcessEntry> content;
  size_t nextEntry;
#elif defined(OS_MACOSX)
  std::vector<kinfo_proc> kinfo_procs_;
  size_t index_of_kinfo_proc_;
#endif
#if !defined(OS_BSD)
  ProcessEntry entry_;
  const ProcessFilter* filter_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(NamedProcessIterator);
};







struct WorkingSetKBytes {
  size_t priv;
  size_t shareable;
  size_t shared;
};







struct CommittedKBytes {
  size_t priv;
  size_t mapped;
  size_t image;
};






struct FreeMBytes {
  size_t total;
  size_t largest;
  void* largest_ptr;
};





class ProcessMetrics {
 public:
  
  
  static ProcessMetrics* CreateProcessMetrics(ProcessHandle process);

  ~ProcessMetrics();

  
  
  size_t GetPagefileUsage() const;
  
  size_t GetPeakPagefileUsage() const;
  
  size_t GetWorkingSetSize() const;
  
  
  
  size_t GetPrivateBytes() const;
  
  
  void GetCommittedKBytes(CommittedKBytes* usage) const;
  
  
  bool GetWorkingSetKBytes(WorkingSetKBytes* ws_usage) const;

  
  
  
  
  bool CalculateFreeMemory(FreeMBytes* free) const;

  
  
  
  
  
  
  int GetCPUUsage();

  
  
  
  
  
  bool GetIOCounters(IoCounters* io_counters) const;

 private:
  explicit ProcessMetrics(ProcessHandle process);

  ProcessHandle process_;

  int processor_count_;

  
  int64_t last_time_;
  int64_t last_system_time_;

  DISALLOW_EVIL_CONSTRUCTORS(ProcessMetrics);
};







bool EnableLowFragmentationHeap();



void EnableTerminationOnHeapCorruption();



void RaiseProcessToHighPriority();

}  

#endif  
