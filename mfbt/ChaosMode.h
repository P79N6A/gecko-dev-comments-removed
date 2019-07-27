





#ifndef mozilla_ChaosMode_h
#define mozilla_ChaosMode_h

#include "mozilla/EnumSet.h"

#include <stdint.h>
#include <stdlib.h>

namespace mozilla {






class ChaosMode
{
public:
  enum ChaosFeature {
    None = 0x0,
    
    ThreadScheduling = 0x1,
    
    NetworkScheduling = 0x2,
    
    TimerScheduling = 0x4,
    
    IOAmounts = 0x8,
    
    HashTableIteration = 0x10,
    Any = 0xffffffff,
  };

private:
  
  static const ChaosFeature sChaosFeatures = None;

public:
  static bool isActive(ChaosFeature aFeature)
  {
    return sChaosFeatures & aFeature;
  }

  



  static uint32_t randomUint32LessThan(uint32_t aBound)
  {
    return uint32_t(rand()) % aBound;
  }
};

} 

#endif
