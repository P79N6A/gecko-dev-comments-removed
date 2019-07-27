





#ifndef jsnativestack_h
#define jsnativestack_h

#include "js/Utility.h"

namespace js {

extern void *
GetNativeStackBaseImpl();

inline uintptr_t
GetNativeStackBase()
{
    uintptr_t stackBase = reinterpret_cast<uintptr_t>(GetNativeStackBaseImpl());
    MOZ_ASSERT(stackBase != 0);
    MOZ_ASSERT(stackBase % sizeof(void *) == 0);
    return stackBase;
}

} 

#endif 
