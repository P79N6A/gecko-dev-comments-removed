



#ifndef SANDBOX_LINUX_SECCOMP_BPF_BASICBLOCK_H__
#define SANDBOX_LINUX_SECCOMP_BPF_BASICBLOCK_H__

#include <vector>

#include "sandbox/linux/seccomp-bpf/instruction.h"

namespace sandbox {

struct BasicBlock {
  BasicBlock();
  ~BasicBlock();

  
  
  
  
  template <class T>
  class Less {
   public:
    Less(const T& data,
         int (*cmp)(const BasicBlock*, const BasicBlock*, const T& data))
        : data_(data), cmp_(cmp) {}

    bool operator()(const BasicBlock* a, const BasicBlock* b) const {
      return cmp_(a, b, data_) < 0;
    }

   private:
    const T& data_;
    int (*cmp_)(const BasicBlock*, const BasicBlock*, const T&);
  };

  
  std::vector<Instruction*> instructions;

  
  
  
  int offset;
};

}  

#endif  
