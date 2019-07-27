





#ifndef mozilla_SandboxAssembler_h
#define mozilla_SandboxAssembler_h

#include <vector>

#include "mozilla/Assertions.h"

struct sock_filter;

namespace mozilla {

class SandboxAssembler {
public:
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
    
    Condition(const Condition& aOther)
    : mSyscallNr(aOther.mSyscallNr)
    , mCheckingArg(aOther.mCheckingArg)
    , mArgChecked(aOther.mArgChecked)
    , mArgValues(aOther.mArgValues)
    { }
  };

  
  void Allow(const Condition& aCond) {
    mRuleStack.push_back(std::make_pair(0, aCond));
  }
  
  
  void Deny(int aErrno, const Condition& aCond) {
    MOZ_ASSERT(aErrno != 0);
    mRuleStack.push_back(std::make_pair(aErrno, aCond));
  }

  void Compile(std::vector<sock_filter>* aProgram, bool aPrint = false);
private:
  std::vector<std::pair<int, Condition>> mRuleStack;

  static const uint8_t sNumArgs = 6;
};

}

#endif
