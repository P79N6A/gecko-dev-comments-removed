



#ifndef SANDBOX_LINUX_SECCOMP_BPF_TRAP_H__
#define SANDBOX_LINUX_SECCOMP_BPF_TRAP_H__

#include <signal.h>
#include <stdint.h>

#include <map>

#include "base/macros.h"
#include "sandbox/linux/bpf_dsl/trap_registry.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {









class SANDBOX_EXPORT Trap : public bpf_dsl::TrapRegistry {
 public:
  virtual uint16_t Add(TrapFnc fnc, const void* aux, bool safe) override;

  virtual bool EnableUnsafeTraps() override;

  
  
  static bpf_dsl::TrapRegistry* Registry();

  
  
  
  
  
  static uint16_t MakeTrap(TrapFnc fnc, const void* aux, bool safe);

  
  
  
  
  
  
  
  
  
  static bool EnableUnsafeTrapsInSigSysHandler();

 private:
  struct TrapKey {
    TrapKey() : fnc(NULL), aux(NULL), safe(false) {}
    TrapKey(TrapFnc f, const void* a, bool s) : fnc(f), aux(a), safe(s) {}
    TrapFnc fnc;
    const void* aux;
    bool safe;
    bool operator<(const TrapKey&) const;
  };
  typedef std::map<TrapKey, uint16_t> TrapIds;

  
  
  Trap();

  
  
  ~Trap();

  static void SigSysAction(int nr, siginfo_t* info, void* void_context);

  
  
  void SigSys(int nr, siginfo_t* info, void* void_context)
      __attribute__((noinline));
  bool SandboxDebuggingAllowedByUser() const;

  
  
  
  
  static Trap* global_trap_;

  TrapIds trap_ids_;            
  TrapKey* trap_array_;         
  size_t trap_array_size_;      
  size_t trap_array_capacity_;  
  bool has_unsafe_traps_;       

  
  
  DISALLOW_COPY_AND_ASSIGN(Trap);
};

}  

#endif  
