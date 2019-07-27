



#ifndef SANDBOX_LINUX_SECCOMP_BPF_ERRORCODE_H__
#define SANDBOX_LINUX_SECCOMP_BPF_ERRORCODE_H__

#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"
#include "sandbox/linux/seccomp-bpf/trap.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {

struct arch_seccomp_data;









class SANDBOX_EXPORT ErrorCode {
 public:
  enum {
    
    
    
    
    ERR_ALLOWED = 0x04000000,

    
    
    
    
    ERR_TRACE   = 0x08000000,

    
    
    
    
    ERR_MIN_ERRNO = 0,
    
    
    ERR_MAX_ERRNO = 4095,
  };

  
  
  
  
  
  
  enum ArgType {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    TP_32BIT,

    
    
    
    
    
    
    
    
    TP_64BIT,
  };

  enum Operation {
    
    OP_EQUAL,

    
    
    
    
    
    
    OP_GREATER_UNSIGNED,
    OP_GREATER_EQUAL_UNSIGNED,

    
    
    
    
    
    OP_HAS_ALL_BITS,
    OP_HAS_ANY_BITS,

    
    OP_NUM_OPS,
  };

  enum ErrorType {
    ET_INVALID,
    ET_SIMPLE,
    ET_TRAP,
    ET_COND,
  };

  
  
  
  
  
  ErrorCode() : error_type_(ET_INVALID), err_(SECCOMP_RET_INVALID) {}
  explicit ErrorCode(int err);

  
  
  
  
  
  

  
  ~ErrorCode() {}

  bool Equals(const ErrorCode& err) const;
  bool LessThan(const ErrorCode& err) const;

  uint32_t err() const { return err_; }
  ErrorType error_type() const { return error_type_; }

  bool safe() const { return safe_; }

  uint64_t value() const { return value_; }
  int argno() const { return argno_; }
  ArgType width() const { return width_; }
  Operation op() const { return op_; }
  const ErrorCode* passed() const { return passed_; }
  const ErrorCode* failed() const { return failed_; }

  struct LessThan {
    bool operator()(const ErrorCode& a, const ErrorCode& b) const {
      return a.LessThan(b);
    }
  };

 private:
  friend class CodeGen;
  friend class SandboxBPF;
  friend class Trap;

  
  
  
  ErrorCode(Trap::TrapFnc fnc, const void* aux, bool safe, uint16_t id);

  
  
  ErrorCode(int argno,
            ArgType width,
            Operation op,
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
      uint64_t value_;           
      int argno_;                
      ArgType width_;            
      Operation op_;             
      const ErrorCode* passed_;  
      const ErrorCode* failed_;  
    };
  };

  
  
  
  uint32_t err_;
};

}  

#endif
