
































#ifndef CLIENT_LINUX_MINIDUMP_WRITER_LINUX_CORE_DUMPER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_LINUX_CORE_DUMPER_H_

#include "client/linux/minidump_writer/linux_dumper.h"
#include "common/linux/elf_core_dump.h"
#include "common/linux/memory_mapped_file.h"

namespace google_breakpad {

class LinuxCoreDumper : public LinuxDumper {
 public:
  
  
  
  
  
  LinuxCoreDumper(pid_t pid, const char* core_path, const char* procfs_path);

  
  
  
  
  
  
  
  
  
  
  
  
  
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
  
  const char* core_path_;

  
  
  const char* procfs_path_;

  
  MemoryMappedFile mapped_core_file_;

  
  ElfCoreDump core_;

  
  wasteful_vector<ThreadInfo> thread_infos_;
};

}  

#endif  
