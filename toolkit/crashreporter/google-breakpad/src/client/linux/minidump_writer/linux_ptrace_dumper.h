

































#ifndef CLIENT_LINUX_MINIDUMP_WRITER_LINUX_PTRACE_DUMPER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_LINUX_PTRACE_DUMPER_H_

#include "client/linux/minidump_writer/linux_dumper.h"

namespace google_breakpad {

class LinuxPtraceDumper : public LinuxDumper {
 public:
  
  
  explicit LinuxPtraceDumper(pid_t pid);

  
  
  
  
  
  virtual bool BuildProcPath(char* path, pid_t pid, const char* node) const;

  
  
  
  
  virtual void CopyFromProcess(void* dest, pid_t child, const void* src,
                               size_t length);

  
  
  
  virtual bool GetThreadInfoByIndex(size_t index, ThreadInfo* info);

  
  
  
  virtual bool IsPostMortem() const;

  
  
  virtual bool ThreadsSuspend();

  
  
  virtual bool ThreadsResume();

 protected:
  
  
  virtual bool EnumerateThreads();

 private:
  
  bool threads_suspended_;
};

}  

#endif  
