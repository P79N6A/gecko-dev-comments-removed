





#ifndef mozilla_SandboxInfo_h
#define mozilla_SandboxInfo_h

#include "mozilla/Types.h"




namespace mozilla {

class SandboxInfo {
public:
  
  SandboxInfo(const SandboxInfo& aOther) : mFlags(aOther.mFlags) { }

  
  static const SandboxInfo& Get() { return sSingleton; }

  enum Flags {
    
    kHasSeccompBPF     = 1 << 0,
    
    kEnabledForContent = 1 << 1,
    
    kEnabledForMedia   = 1 << 2,
    
    kVerbose           = 1 << 3,
  };

  bool Test(Flags aFlag) const { return (mFlags & aFlag) == aFlag; }

  
  bool CanSandboxContent() const
  {
    return !Test(kEnabledForContent) || Test(kHasSeccompBPF);
  }

  
  bool CanSandboxMedia() const
  {
    return !Test(kEnabledForMedia) || Test(kHasSeccompBPF);
  }
private:
  enum Flags mFlags;
  static MOZ_EXPORT const SandboxInfo sSingleton;
  SandboxInfo();
};

} 

#endif 
