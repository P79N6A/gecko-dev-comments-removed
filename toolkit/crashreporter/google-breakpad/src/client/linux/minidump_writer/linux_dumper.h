




































#ifndef CLIENT_LINUX_MINIDUMP_WRITER_LINUX_DUMPER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_LINUX_DUMPER_H_

#include <elf.h>
#include <linux/limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>

#include "common/memory.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

#if defined(__i386) || defined(__x86_64)
typedef typeof(((struct user*) 0)->u_debugreg[0]) debugreg_t;
#endif


#if defined(__i386) || defined(__ARM_EABI__)
typedef Elf32_auxv_t elf_aux_entry;
#elif defined(__x86_64)
typedef Elf64_auxv_t elf_aux_entry;
#endif

typedef typeof(((elf_aux_entry*) 0)->a_un.a_val) elf_aux_val_t;




const char kLinuxGateLibraryName[] = "linux-gate.so";


struct ThreadInfo {
  pid_t tgid;   
  pid_t ppid;   

  uintptr_t stack_pointer;  


#if defined(__i386) || defined(__x86_64)
  user_regs_struct regs;
  user_fpregs_struct fpregs;
  static const unsigned kNumDebugRegisters = 8;
  debugreg_t dregs[8];
#if defined(__i386)
  user_fpxregs_struct fpxregs;
#endif  

#elif defined(__ARM_EABI__)
  
  struct user_regs regs;
  struct user_fpregs fpregs;
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

  virtual ~LinuxDumper();

  
  virtual bool Init();

  
  virtual bool IsPostMortem() const = 0;

  
  virtual bool ThreadsSuspend() = 0;
  virtual bool ThreadsResume() = 0;

  
  
  virtual bool GetThreadInfoByIndex(size_t index, ThreadInfo* info) = 0;

  
  const wasteful_vector<pid_t> &threads() { return threads_; }
  const wasteful_vector<MappingInfo*> &mappings() { return mappings_; }
  const MappingInfo* FindMapping(const void* address) const;
  const wasteful_vector<elf_aux_val_t>& auxv() { return auxv_; }

  
  
  
  
  bool GetStackInfo(const void** stack, size_t* stack_len, uintptr_t stack_top);

  PageAllocator* allocator() { return &allocator_; }

  
  
  virtual void CopyFromProcess(void* dest, pid_t child, const void* src,
                               size_t length) = 0;

  
  
  
  
  virtual bool BuildProcPath(char* path, pid_t pid, const char* node) const = 0;

  
  
  bool ElfFileIdentifierForMapping(const MappingInfo& mapping,
                                   bool member,
                                   unsigned int mapping_id,
                                   uint8_t identifier[sizeof(MDGUID)]);

  uintptr_t crash_address() const { return crash_address_; }
  void set_crash_address(uintptr_t crash_address) {
    crash_address_ = crash_address;
  }

  int crash_signal() const { return crash_signal_; }
  void set_crash_signal(int crash_signal) { crash_signal_ = crash_signal; }

  pid_t crash_thread() const { return crash_thread_; }
  void set_crash_thread(pid_t crash_thread) { crash_thread_ = crash_thread; }

 protected:
  bool ReadAuxv();

  virtual bool EnumerateMappings();

  virtual bool EnumerateThreads() = 0;

  
  
  
  
  
  
  
  
  
  bool HandleDeletedFileInMapping(char* path) const;

   
  const pid_t pid_;

  
  uintptr_t crash_address_;

  
  int crash_signal_;

  
  pid_t crash_thread_;

  mutable PageAllocator allocator_;

  
  wasteful_vector<pid_t> threads_;

  
  wasteful_vector<MappingInfo*> mappings_;

  
  wasteful_vector<elf_aux_val_t> auxv_;
};

}  

#endif  
