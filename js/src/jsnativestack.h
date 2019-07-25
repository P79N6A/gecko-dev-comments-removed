





































#ifndef jsnativestack_h__
#define jsnativestack_h__

#include "jspubtd.h"
#include "jsutil.h"

namespace js {

extern void *
GetNativeStackBaseImpl();

inline jsuword *
GetNativeStackBase()
{
    void *stackBase = GetNativeStackBaseImpl();
    JS_ASSERT(reinterpret_cast<jsuword>(stackBase) % sizeof(void *) == 0);
    return static_cast<jsuword *>(stackBase);
}

} 

#endif 
