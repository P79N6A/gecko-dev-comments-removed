







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

MOZCE_SHUNT_API char* mozce_strerror(int inErrno)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_strerror called\n");
#endif
    return "Unknown Error";
}


MOZCE_SHUNT_API int mozce_wsprintfA(LPTSTR lpOut, LPCTSTR lpFmt, ... )
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_wsprintfA called\n");
#endif
    return 0;
}

#if 0
{
#endif
} 

