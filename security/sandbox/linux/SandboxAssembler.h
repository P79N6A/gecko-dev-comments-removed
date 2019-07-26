





#ifndef mozilla_SandboxAssembler_h
#define mozilla_SandboxAssembler_h

#include "sandbox/linux/seccomp-bpf/codegen.h"

#include <vector>
#include "mozilla/Assertions.h"

using namespace sandbox;

namespace mozilla {

class SandboxAssembler {
public:
  SandboxAssembler();
  ~SandboxAssembler();

  class Condition {
    friend class SandboxAssembler;
    uint32_t mSyscallNr;
    bool mCheckingArg;
    uint8_t mArgChecked;
    
    
    std::vector<uint32_t> mArgValues;
  public:
    
    explicit Condition(uint32_t aSyscallNr)
      : mSyscallNr(aSyscallNr)
      , mCheckingArg(false)
    { }
    
    
    
    
    template<size_t n>
    Condition(uint32_t aSyscallNr, uint8_t aArgChecked,
              const uint32_t (&aArgValues)[n])
      : mSyscallNr(aSyscallNr)
      , mCheckingArg(true)
      , mArgChecked(aArgChecked)
      , mArgValues(aArgValues, aArgValues + n)
    {
      MOZ_ASSERT(aArgChecked < sNumArgs);
    }
  };

  
  void Allow(const Condition &aCond) {
    Handle(aCond, RetAllow());
  }
  
  
  void Deny(int aErrno, const Condition &aCond) {
    Handle(aCond, RetDeny(aErrno));
  }

  void Finish();
  void Compile(std::vector<struct sock_filter> *aProgram,
               bool aPrint = false);
private:
  CodeGen mCode;
  
  Instruction *mHead;
  
  
  Instruction *mTail;
  
  
  
  Instruction *mTailAlt;

  Instruction *RetAllow();
  Instruction *RetDeny(int aErrno);
  Instruction *RetKill();
  Instruction *LoadArch(Instruction *aNext);
  Instruction *LoadSyscall(Instruction *aNext);
  Instruction *LoadArgHi(int aArg, Instruction *aNext);
  Instruction *LoadArgLo(int aArg, Instruction *aNext);
  Instruction *JmpEq(uint32_t aValue, Instruction *aThen, Instruction *aElse);
  void AppendCheck(Instruction *aCheck,
                   Instruction *aNewTail,
                   Instruction *aNewTailAlt);
  void Handle(const Condition &aCond, Instruction* aResult);

  static const uint8_t sNumArgs = 6;
};

}

#endif
