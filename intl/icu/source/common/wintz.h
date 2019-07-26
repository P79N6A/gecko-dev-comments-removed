










#ifndef __WINTZ
#define __WINTZ

#include "unicode/utypes.h"

#if U_PLATFORM_HAS_WIN32_API






U_CDECL_BEGIN

typedef struct _TIME_ZONE_INFORMATION TIME_ZONE_INFORMATION;
U_CDECL_END

U_CFUNC const char* U_EXPORT2
uprv_detectWindowsTimeZone();

#endif 

#endif 
