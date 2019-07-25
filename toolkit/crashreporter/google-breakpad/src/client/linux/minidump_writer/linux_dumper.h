




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_LINUX_DUMPER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_LINUX_DUMPER_H_

#include <elf.h>
#include <linux/limits.h>
#include <stdint.h>
#include <sys/types.h>
#if !defined(__ANDROID__)
#include <sys/user.h>
#endif

#include "common/memory.h"
#include "google_breakpad/common/minidump_format.h"
#include <asm/ptrace.h>

namespace google_breakpad {

#if defined(__i386) || defined(__x86_64)
typedef typeof(((struct user*) 0)->u_debugreg[0]) debugreg_t;
#endif


#if defined(__i386) || defined(__ARM_EABI__)
#if !defined(__ANDROID__)
typedef Elf32_auxv_t elf_aux_entry;
#else

typedef struct
{
  uint32_t a_type;              
  union
    {
      uint32_t a_val;           
    } a_un;
} elf_aux_entry;

#if !defined(AT_SYSINFO_EHDR)
#define AT_SYSINFO_EHDR 33
#endif
#endif  
#elif defined(__x86_64__)
typedef Elf64_auxv_t elf_aux_entry;
#endif



const char kLinuxGateLibraryName[] = "linux-gate.so";


struct ThreadInfo {
  pid_t tid;    
  pid_t tgid;   
  pid_t ppid;   

  
  
  const void* stack;  
  size_t stack_len;  


#if defined(__i386) || defined(__x86_64)
  user_regs_struct regs;
  user_fpregs_struct fpregs;
  static const unsigned kNumDebugRegisters = 8;
  debugreg_t dregs[8];
#if defined(__i386)
  user_fpxregs_struct fpxregs;
#endif  

#elif defined(__ARM_EABI__)
  
#if defined(__ANDROID__)
  struct pt_regs regs;
#else
  struct user_regs regs;
  struct user_fpregs fpregs;
#endif  
#endif
};



struct MappingInfo {
  uintptr_t start_addr;
  size_t size;
  size_t offset;  
  char name[NAME_MAX];
};


bool AttachThread(pid_t pid);


bool DetachThread(pid_t pid);



bool GetThreadRegisters(ThreadInfo* info);

class LinuxDumper {
 public:
  explicit LinuxDumper(pid_t pid);

  
  bool Init();

  
  bool ThreadsAttach();
  bool ThreadsDetach();

  
  
  bool ThreadInfoGet(ThreadInfo* info);

  
  const wasteful_vector<pid_t> &threads() { return threads_; }
  const wasteful_vector<MappingInfo*> &mappings() { return mappings_; }
  const MappingInfo* FindMapping(const void* address) const;

  
  
  
  
  bool GetStackInfo(const void** stack, size_t* stack_len, uintptr_t stack_top);

  PageAllocator* allocator() { return &allocator_; }

  
  static void CopyFromProcess(void* dest, pid_t child, const void* src,
                              size_t length);

  
  
  
  void BuildProcPath(char* path, pid_t pid, const char* node) const;

  
  bool ElfFileIdentifierForMapping(const MappingInfo& mapping,
                                   uint8_t identifier[sizeof(MDGUID)]);

  
  
  
  
  
  void* FindBeginningOfLinuxGateSharedLibrary(const pid_t pid) const;
 private:
  bool EnumerateMappings(wasteful_vector<MappingInfo*>* result) const;
  bool EnumerateThreads(wasteful_vector<pid_t>* result) const;

  const pid_t pid_;

  mutable PageAllocator allocator_;

  bool threads_suspended_;
  wasteful_vector<pid_t> threads_;  
  wasteful_vector<MappingInfo*> mappings_;  
};

}  

#endif  
