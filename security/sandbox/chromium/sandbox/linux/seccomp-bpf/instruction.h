



#ifndef SANDBOX_LINUX_SECCOMP_BPF_INSTRUCTION_H__
#define SANDBOX_LINUX_SECCOMP_BPF_INSTRUCTION_H__

#include <stdint.h>

#include <cstddef>

namespace sandbox {















struct Instruction {
  
  
  Instruction(uint16_t c, uint32_t parm, Instruction* n)
      : code(c), jt(0), jf(0), jt_ptr(NULL), jf_ptr(NULL), next(n), k(parm) {}

  
  Instruction(uint16_t c, uint32_t parm, Instruction* jt, Instruction* jf)
      : code(c), jt(0), jf(0), jt_ptr(jt), jf_ptr(jf), next(NULL), k(parm) {}

  uint16_t code;

  
  
  uint8_t jt, jf;

  
  
  
  
  Instruction* jt_ptr, *jf_ptr;

  
  
  
  Instruction* next;

  uint32_t k;
};

}  

#endif  
