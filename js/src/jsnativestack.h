





































#ifndef jsnativestack_h__
#define jsnativestack_h__

#include "jspubtd.h"
#include "jsutil.h"

namespace js {

extern void *
GetNativeStackBaseImpl();

inline uintptr_t *
GetNativeStackBase()
{
    void *stackBase = GetNativeStackBaseImpl();
    JS_ASSERT(reinterpret_cast<uintptr_t>(stackBase) % sizeof(void *) == 0);
    return static_cast<uintptr_t *>(stackBase);
}

} 

#endif 
