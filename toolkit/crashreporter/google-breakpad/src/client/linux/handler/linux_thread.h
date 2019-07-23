






























#ifndef CLIENT_LINUX_HANDLER_LINUX_THREAD_H__
#define CLIENT_LINUX_HANDLER_LINUX_THREAD_H__

#include <stdint.h>
#include <sys/user.h>

namespace google_breakpad {


#define kMaxModuleNameLength 256


struct ThreadInfo {
  
  int tgid;
  
  int pid;
  
  int ppid;
};


struct ModuleInfo {
  char name[kMaxModuleNameLength];
  uintptr_t start_addr;
  int size;
};


struct DebugRegs {
  int dr0;
  int dr1;
  int dr2;
  int dr3;
  int dr4;
  int dr5;
  int dr6;
  int dr7;
};




typedef bool (*ThreadCallback)(const ThreadInfo &thread_info, void *context);




typedef bool (*ModuleCallback)(const ModuleInfo &module_info, void *context);


template<class CallbackFunc>
struct CallbackParam {
  
  CallbackFunc call_back;
  
  void *context;

  CallbackParam() : call_back(NULL), context(NULL) {
  }

  CallbackParam(CallbackFunc func, void *func_context) :
    call_back(func), context(func_context) {
  }
};




















class LinuxThread {
 public:
  
  explicit LinuxThread(int pid);
  ~LinuxThread();

  
  
  
  int SuspendAllThreads();

  
  void ResumeAllThreads() const;

  
  
  int GetThreadCount() const;

  
  
  
  
  int ListThreads(CallbackParam<ThreadCallback> *thread_callback_param) const;

  
  
  bool GetRegisters(int pid, user_regs_struct *regs) const;

  
  
  bool GetFPRegisters(int pid, user_fpregs_struct *regs) const;

  
  
  
  bool GetFPXRegisters(int pid, user_fpxregs_struct *regs) const;

  
  
  bool GetDebugRegisters(int pid, DebugRegs *regs) const;

  
  int GetThreadStackDump(uintptr_t current_ebp,
                         uintptr_t current_esp,
                         void *buf,
                         int buf_size) const;

  
  int GetModuleCount() const;

  
  
  
  
  int ListModules(CallbackParam<ModuleCallback> *callback_param) const;

  
  uintptr_t GetThreadStackBottom(uintptr_t current_ebp) const;

  
  bool FindSigContext(uintptr_t sighandler_ebp, struct sigcontext **sig_ctx);

 private:
  
  typedef bool (*PidCallback)(int pid, void *context);

  
  
  
  
  
  int IterateProcSelfTask(int pid,
                          CallbackParam<PidCallback> *callback_param) const;

  
  bool IsAddressMapped(uintptr_t address) const;

 private:
  
  int pid_;

  
  bool threads_suspened_;
};

}  

#endif  
