




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_LINUX_DUMPER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_LINUX_DUMPER_H_

#include <elf.h>
#include <linux/limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>

#include "common/linux/memory.h"

namespace google_breakpad {

typedef typeof(((struct user*) 0)->u_debugreg[0]) debugreg_t;


#if defined(__i386)
typedef Elf32_auxv_t elf_aux_entry;
#elif defined(__x86_64__)
typedef Elf64_auxv_t elf_aux_entry;
#endif



const char kLinuxGateLibraryName[] = "linux-gate.so";


struct ThreadInfo {
  pid_t tgid;   
  pid_t ppid;   

  
  
  const void* stack;  
  size_t stack_len;  

  user_regs_struct regs;
  user_fpregs_struct fpregs;
#if defined(__i386)
  user_fpxregs_struct fpxregs;
#endif

#if defined(__i386) || defined(__x86_64)

  static const unsigned kNumDebugRegisters = 8;
  debugreg_t dregs[8];
#endif
};



struct MappingInfo {
  uintptr_t start_addr;
  size_t size;
  size_t offset;  
  char name[NAME_MAX];
};

class LinuxDumper {
 public:
  explicit LinuxDumper(pid_t pid);

  
  bool Init();

  
  bool ThreadsSuspend();
  bool ThreadsResume();

  
  
  bool ThreadInfoGet(pid_t tid, ThreadInfo* info);

  
  const wasteful_vector<pid_t> &threads() { return threads_; }
  const wasteful_vector<MappingInfo*> &mappings() { return mappings_; }
  const MappingInfo* FindMapping(const void* address) const;

  
  
  
  
  bool GetStackInfo(const void** stack, size_t* stack_len, uintptr_t stack_top);

  PageAllocator* allocator() { return &allocator_; }

  
  static void CopyFromProcess(void* dest, pid_t child, const void* src,
                              size_t length);

  
  
  
  void BuildProcPath(char* path, pid_t pid, const char* node) const;

  
  
  
  
  
  void* FindBeginningOfLinuxGateSharedLibrary(const pid_t pid) const;
 private:
  bool EnumerateMappings(wasteful_vector<MappingInfo*>* result) const;
  bool EnumerateThreads(wasteful_vector<pid_t>* result) const;

  const pid_t pid_;

  mutable PageAllocator allocator_;

  bool threads_suspened_;
  wasteful_vector<pid_t> threads_;  
  wasteful_vector<MappingInfo*> mappings_;  
};

}  

#endif  
