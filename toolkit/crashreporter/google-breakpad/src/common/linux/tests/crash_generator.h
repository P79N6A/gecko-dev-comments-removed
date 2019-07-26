































#ifndef COMMON_LINUX_TESTS_CRASH_GENERATOR_H_
#define COMMON_LINUX_TESTS_CRASH_GENERATOR_H_

#include <sys/resource.h>

#include <string>

#include "common/tests/auto_tempdir.h"
#include "common/using_std_string.h"

namespace google_breakpad {







class CrashGenerator {
 public:
  CrashGenerator();

  ~CrashGenerator();

  
  
  
  bool HasDefaultCorePattern() const;

  
  string GetCoreFilePath() const;

  
  string GetDirectoryOfProcFilesCopy() const;

  
  
  
  
  bool CreateChildCrash(unsigned num_threads, unsigned crash_thread,
                        int crash_signal, pid_t* child_pid);

  
  
  pid_t GetThreadId(unsigned index) const;

 private:
  
  
  
  bool CopyProcFiles(pid_t pid, const char* path) const;

  
  void CreateThreadsInChildProcess(unsigned num_threads);

  
  
  bool SetCoreFileSizeLimit(rlim_t limit) const;

  
  
  bool MapSharedMemory(size_t memory_size);

  
  
  bool UnmapSharedMemory();

  
  
  pid_t* GetThreadIdPointer(unsigned index);

  
  AutoTempDir temp_dir_;

  
  
  void* shared_memory_;

  
  size_t shared_memory_size_;
};

}  

#endif  
