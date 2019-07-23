






























#ifndef CLIENT_SOLARIS_HANDLER_SOLARIS_LWP_H__
#define CLIENT_SOLARIS_HANDLER_SOLARIS_LWP_H__

#if defined(sparc) || defined(__sparc)
#define TARGET_CPU_SPARC 1
#elif defined(i386) || defined(__i386)
#define TARGET_CPU_X86 1
#else
#error "cannot determine cpu type"
#endif

#include <signal.h>
#include <stdint.h>
#include <sys/user.h>
#include <ucontext.h>

#ifndef _KERNEL
#define _KERNEL
#define MUST_UNDEF_KERNEL
#endif  
#include <sys/procfs.h>
#ifdef MUST_UNDEF_KERNEL
#undef _KERNEL
#undef MUST_UNDEF_KERNEL
#endif  

namespace google_breakpad {


static const int kMaxModuleNameLength = 256;


struct ModuleInfo {
  char name[kMaxModuleNameLength];
  uintptr_t start_addr;
  int size;
};




typedef bool (*LwpCallback)(lwpstatus_t* lsp, void *context); 




typedef bool (*ModuleCallback)(const ModuleInfo &module_info, void *context);




typedef bool (*LwpidCallback)(int lwpid, void *context);


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











class SolarisLwp {
 public:
  
  explicit SolarisLwp(int pid);
  ~SolarisLwp();

  int getpid() const { return this->pid_; }

  
  
  
  int ControlAllLwps(bool suspend);

  
  
  int GetLwpCount() const;

  
  
  
  
  int Lwp_iter_all(int pid, CallbackParam<LwpCallback> *callback_param) const;

  
  int GetModuleCount() const;

  
  
  
  
  int ListModules(CallbackParam<ModuleCallback> *callback_param) const;

  
  uintptr_t GetLwpStackBottom(uintptr_t current_esp) const;

  
  bool FindSigContext(uintptr_t sighandler_ebp, ucontext_t **sig_ctx);

 private:
  
  bool IsAddressMapped(uintptr_t address) const;

 private:
  
  int pid_;
};

}  

#endif  
