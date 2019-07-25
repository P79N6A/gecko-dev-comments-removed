




































#ifndef StackWalk_h_
#define StackWalk_h_


#include "nsStackWalk.h"

namespace mozilla {

nsresult
FramePointerStackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
                      void *aClosure, void **bp);

}

#endif 
