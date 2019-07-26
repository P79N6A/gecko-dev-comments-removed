






#ifndef StackWalk_h_
#define StackWalk_h_


#include "nsStackWalk.h"

namespace mozilla {

nsresult
FramePointerStackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
                      uint32_t aMaxFrames, void* aClosure, void** aBp,
                      void* aStackEnd);

}

#endif 
