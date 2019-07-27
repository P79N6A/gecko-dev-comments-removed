





#ifndef mozilla_ChaosMode_h
#define mozilla_ChaosMode_h

#include "mozilla/Atomics.h"
#include "mozilla/EnumSet.h"

#include <stdint.h>
#include <stdlib.h>

namespace mozilla {

enum ChaosFeature {
  None = 0x0,
  
  ThreadScheduling = 0x1,
  
  NetworkScheduling = 0x2,
  
  TimerScheduling = 0x4,
  
  IOAmounts = 0x8,
  
  HashTableIteration = 0x10,
  Any = 0xffffffff,
};

namespace detail {
extern MFBT_DATA Atomic<uint32_t> gChaosModeCounter;
extern MFBT_DATA ChaosFeature gChaosFeatures;
} 






class ChaosMode
{
public:
  static void SetChaosFeature(ChaosFeature aChaosFeature)
  {
    detail::gChaosFeatures = aChaosFeature;
  }

  static bool isActive(ChaosFeature aFeature)
  {
    if (detail::gChaosModeCounter > 0) {
      return true;
    }
    return detail::gChaosFeatures & aFeature;
  }

  





  static void enterChaosMode()
  {
    detail::gChaosModeCounter++;
  }

  


  static void leaveChaosMode()
  {
    MOZ_ASSERT(detail::gChaosModeCounter > 0);
    detail::gChaosModeCounter--;
  }

  



  static uint32_t randomUint32LessThan(uint32_t aBound)
  {
    MOZ_ASSERT(aBound != 0);
    return uint32_t(rand()) % aBound;
  }
};

} 

#endif 
