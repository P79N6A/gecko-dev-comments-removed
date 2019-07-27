






#ifndef SkThreadPriv_DEFINED
#define SkThreadPriv_DEFINED

#include "SkTypes.h"







static void* sk_atomic_cas(void** addr, void* before, void* after);

#include SK_ATOMICS_PLATFORM_H

#endif
