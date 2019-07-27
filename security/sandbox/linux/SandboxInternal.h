





#ifndef mozilla_SandboxInternal_h
#define mozilla_SandboxInternal_h




#include <signal.h>

#include "mozilla/Types.h"

namespace mozilla {

typedef void (*SandboxCrashFunc)(int, siginfo_t*, void*);
extern MOZ_EXPORT SandboxCrashFunc gSandboxCrashFunc;

} 

#endif 
