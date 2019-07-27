










#ifndef __WINUTIL
#define __WINUTIL

#include "unicode/utypes.h"

#if U_PLATFORM_HAS_WIN32_API

#if !UCONFIG_NO_FORMATTING






class Win32Utilities
{
public:
    struct LCIDRecord
    {
        int32_t lcid;
        char *localeID;
    };

    static LCIDRecord *getLocales(int32_t &localeCount);
    static void freeLocales(LCIDRecord *records);

private:
    Win32Utilities();
};

#endif 

#endif 

#endif 
