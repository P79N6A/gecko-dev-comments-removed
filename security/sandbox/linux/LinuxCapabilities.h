





#ifndef mozilla_LinuxCapabilities_h
#define mozilla_LinuxCapabilities_h

#include <linux/capability.h>
#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/PodOperations.h"








#ifndef _LINUX_CAPABILITY_VERSION_3
#define _LINUX_CAPABILITY_VERSION_3  0x20080522
#define _LINUX_CAPABILITY_U32S_3     2
#endif
#ifndef CAP_TO_INDEX
#define CAP_TO_INDEX(x)     ((x) >> 5)
#define CAP_TO_MASK(x)      (1 << ((x) & 31))
#endif

namespace mozilla {

class LinuxCapabilities final
{
public:
  
  class BitRef {
    __u32& mWord;
    __u32 mMask;
    friend class LinuxCapabilities;
    BitRef(__u32& aWord, uint32_t aMask) : mWord(aWord), mMask(aMask) { }
    BitRef(const BitRef& aBit) : mWord(aBit.mWord), mMask(aBit.mMask) { }
  public:
    operator bool() const {
      return mWord & mMask;
    }
    BitRef& operator=(bool aSetTo) {
      if (aSetTo) {
        mWord |= mMask;
      } else {
        mWord &= mMask;
      }
      return *this;
    }
  };

  
  LinuxCapabilities() { PodArrayZero(mBits); }

  
  
  
  bool GetCurrent();

  
  
  
  bool SetCurrentRaw() const;

  
  
  
  
  bool SetCurrent() {
    Normalize();
    return SetCurrentRaw();
  }

  void Normalize() {
    for (size_t i = 0; i < _LINUX_CAPABILITY_U32S_3; ++i) {
      mBits[i].permitted |= mBits[i].effective | mBits[i].inheritable;
    }
  }

  
  
  
  
  BitRef Effective(unsigned aCap)
  {
    return GenericBitRef(&__user_cap_data_struct::effective, aCap);
  }

  BitRef Permitted(unsigned aCap)
  {
    return GenericBitRef(&__user_cap_data_struct::permitted, aCap);
  }

  BitRef Inheritable(unsigned aCap)
  {
    return GenericBitRef(&__user_cap_data_struct::inheritable, aCap);
  }

private:
  __user_cap_data_struct mBits[_LINUX_CAPABILITY_U32S_3];

  BitRef GenericBitRef(__u32 __user_cap_data_struct::* aField, unsigned aCap)
  {
    
    MOZ_ASSERT(CAP_TO_INDEX(aCap) < _LINUX_CAPABILITY_U32S_3);
    return BitRef(mBits[CAP_TO_INDEX(aCap)].*aField, CAP_TO_MASK(aCap));
  }
};

} 

#endif 
