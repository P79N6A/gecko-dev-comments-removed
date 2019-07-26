



#ifndef SANDBOX_LINUX_SECCOMP_BPF_TRAP_H__
#define SANDBOX_LINUX_SECCOMP_BPF_TRAP_H__

#include <signal.h>
#include <stdint.h>

#include <map>
#include <vector>

#include "base/basictypes.h"
#include "sandbox/linux/sandbox_export.h"

namespace sandbox {

class ErrorCode;









class SANDBOX_EXPORT Trap {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef intptr_t (*TrapFnc)(const struct arch_seccomp_data& args, void* aux);

  
  
  
  
  static ErrorCode MakeTrap(TrapFnc fnc, const void* aux, bool safe);

  
  
  
  
  
  
  
  
  static bool EnableUnsafeTrapsInSigSysHandler();

  
  static ErrorCode ErrorCodeFromTrapId(uint16_t id);

 private:
  
  
  ~Trap();

  struct TrapKey {
    TrapKey(TrapFnc f, const void* a, bool s) : fnc(f), aux(a), safe(s) {}
    TrapFnc fnc;
    const void* aux;
    bool safe;
    bool operator<(const TrapKey&) const;
  };
  typedef std::map<TrapKey, uint16_t> TrapIds;

  
  
  
  
  
  
  
  static Trap* GetInstance();
  static void SigSysAction(int nr, siginfo_t* info, void* void_context);

  
  
  void SigSys(int nr, siginfo_t* info, void* void_context)
      __attribute__((noinline));
  ErrorCode MakeTrapImpl(TrapFnc fnc, const void* aux, bool safe);
  bool SandboxDebuggingAllowedByUser() const;

  
  
  
  
  static Trap* global_trap_;

  TrapIds trap_ids_;            
  ErrorCode* trap_array_;       
  size_t trap_array_size_;      
  size_t trap_array_capacity_;  
  bool has_unsafe_traps_;       

  
  
  
  
  DISALLOW_IMPLICIT_CONSTRUCTORS(Trap);
};

}  

#endif  
