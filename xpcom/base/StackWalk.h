






#ifndef StackWalk_h_
#define StackWalk_h_


#include "nsStackWalk.h"

namespace mozilla {

nsresult
FramePointerStackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
                      void *aClosure, void **bp, void *stackEnd);

}

#endif 
