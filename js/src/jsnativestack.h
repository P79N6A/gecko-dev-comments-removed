





#ifndef jsnativestack_h__
#define jsnativestack_h__

#include "js/Utility.h"

namespace js {

extern void *
GetNativeStackBaseImpl();

inline uintptr_t
GetNativeStackBase()
{
    uintptr_t stackBase = reinterpret_cast<uintptr_t>(GetNativeStackBaseImpl());
    JS_ASSERT(stackBase % sizeof(void *) == 0);
    return stackBase;
}

} 

#endif 
