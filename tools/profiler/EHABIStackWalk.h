











#ifndef mozilla_EHABIStackWalk_h__
#define mozilla_EHABIStackWalk_h__

#include <stddef.h>

#ifdef ANDROID
# include "android-signal-defs.h"
#else
# include <ucontext.h>
#endif

namespace mozilla {

void EHABIStackWalkInit();

size_t EHABIStackWalk(const mcontext_t &aContext, void *stackBase,
                      void **aSPs, void **aPCs, size_t aNumFrames);

}

#endif
