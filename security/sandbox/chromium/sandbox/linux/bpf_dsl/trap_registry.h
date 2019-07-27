



#ifndef SANDBOX_LINUX_BPF_DSL_TRAP_REGISTRY_H_
#define SANDBOX_LINUX_BPF_DSL_TRAP_REGISTRY_H_

#include <stdint.h>

#include "base/macros.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {


struct arch_seccomp_data {
  int nr;
  uint32_t arch;
  uint64_t instruction_pointer;
  uint64_t args[6];
};

namespace bpf_dsl {




class SANDBOX_EXPORT TrapRegistry {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  typedef intptr_t (*TrapFnc)(const struct arch_seccomp_data& args, void* aux);

  
  
  
  
  virtual uint16_t Add(TrapFnc fnc, const void* aux, bool safe) = 0;

  
  
  virtual bool EnableUnsafeTraps() = 0;

 protected:
  TrapRegistry() {}
  ~TrapRegistry() {}

  DISALLOW_COPY_AND_ASSIGN(TrapRegistry);
};

}  
}  

#endif  
