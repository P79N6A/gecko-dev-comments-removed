







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

MOZCE_SHUNT_API char* strerror(int inErrno)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("strerror called\n");
#endif
    return "Unknown Error";
}

#if 0
{
#endif
} 

