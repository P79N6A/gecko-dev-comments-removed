



#ifndef SANDBOX_LINUX_SECCOMP_BPF_INSTRUCTION_H__
#define SANDBOX_LINUX_SECCOMP_BPF_INSTRUCTION_H__

#include <stdint.h>

namespace sandbox {















struct Instruction {
  
  
  Instruction(uint16_t c, uint32_t parm, Instruction* n)
      : code(c), next(n), k(parm) {}

  
  Instruction(uint16_t c, uint32_t parm, Instruction* jt, Instruction* jf)
      : code(c), jt_ptr(jt), jf_ptr(jf), k(parm) {}

  uint16_t code;
  union {
    
    
    struct {
      uint8_t jt, jf;
    };

    
    
    
    
    struct {
      Instruction* jt_ptr, *jf_ptr;
    };

    
    
    
    Instruction* next;
  };
  uint32_t k;
};

}  

#endif  
