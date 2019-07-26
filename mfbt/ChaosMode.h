





#ifndef mozilla_ChaosMode_h
#define mozilla_ChaosMode_h

#include <stdint.h>
#include <stdlib.h>

namespace mozilla {






class ChaosMode
{
public:
  static bool isActive()
  {
    
    return false;
  }

  



  static uint32_t randomUint32LessThan(uint32_t aBound)
  {
    return uint32_t(rand()) % aBound;
  }
};

} 

#endif 
