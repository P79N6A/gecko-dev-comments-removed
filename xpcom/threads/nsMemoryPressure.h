





#ifndef nsMemoryPressure_h__
#define nsMemoryPressure_h__

#include "nscore.h"

enum MemoryPressureState
{
  


  MemPressure_None = 0,

  






  MemPressure_New,

  














  MemPressure_Ongoing
};





MemoryPressureState
NS_GetPendingMemoryPressure();










void
NS_DispatchEventualMemoryPressure(MemoryPressureState aState);










nsresult
NS_DispatchMemoryPressure(MemoryPressureState aState);

#endif 
