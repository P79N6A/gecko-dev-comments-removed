



#ifndef SANDBOX_LINUX_SECCOMP_BPF_ERRORCODE_H__
#define SANDBOX_LINUX_SECCOMP_BPF_ERRORCODE_H__

#include "sandbox/linux/seccomp-bpf/trap.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace bpf_dsl {
class PolicyCompiler;
}









class SANDBOX_EXPORT ErrorCode {
 public:
  enum {
    
    
    
    
    ERR_ALLOWED = 0x04000000,

    
    
    
    
    ERR_TRACE   = 0x08000000,

    
    
    
    
    ERR_MIN_ERRNO = 0,
#if defined(__mips__)
    
    ERR_MAX_ERRNO = 1133,
#else
    
    
    ERR_MAX_ERRNO = 4095,
#endif
  };

  
  
  
  
  
  
  enum ArgType {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    TP_32BIT,

    
    
    
    
    
    
    
    
    TP_64BIT,
  };

  
  enum Operation {
    
    OP_EQUAL,

    
    
    
    
    
    OP_HAS_ALL_BITS,
    OP_HAS_ANY_BITS,
  };

  enum ErrorType {
    ET_INVALID,
    ET_SIMPLE,
    ET_TRAP,
    ET_COND,
  };

  
  
  
  
  
  ErrorCode();
  explicit ErrorCode(int err);

  
  
  
  
  
  

  
  ~ErrorCode() {}

  bool Equals(const ErrorCode& err) const;
  bool LessThan(const ErrorCode& err) const;

  uint32_t err() const { return err_; }
  ErrorType error_type() const { return error_type_; }

  bool safe() const { return safe_; }

  uint64_t mask() const { return mask_; }
  uint64_t value() const { return value_; }
  int argno() const { return argno_; }
  ArgType width() const { return width_; }
  const ErrorCode* passed() const { return passed_; }
  const ErrorCode* failed() const { return failed_; }

  struct LessThan {
    bool operator()(const ErrorCode& a, const ErrorCode& b) const {
      return a.LessThan(b);
    }
  };

 private:
  friend bpf_dsl::PolicyCompiler;
  friend class CodeGen;
  friend class SandboxBPF;
  friend class Trap;

  
  
  
  ErrorCode(uint16_t trap_id, Trap::TrapFnc fnc, const void* aux, bool safe);

  
  
  ErrorCode(int argno,
            ArgType width,
            uint64_t mask,
            uint64_t value,
            const ErrorCode* passed,
            const ErrorCode* failed);

  ErrorType error_type_;

  union {
    
    struct {
      Trap::TrapFnc fnc_;  
      void* aux_;          
      bool safe_;          
    };

    
    struct {
      uint64_t mask_;            
      uint64_t value_;           
      int argno_;                
      ArgType width_;            
      const ErrorCode* passed_;  
      const ErrorCode* failed_;  
    };
  };

  
  
  
  uint32_t err_;
};

}  

#endif
