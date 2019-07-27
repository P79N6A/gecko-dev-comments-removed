










#ifndef __WINDTTST
#define __WINDTTST

#include "unicode/utypes.h"

#if U_PLATFORM_HAS_WIN32_API

#if !UCONFIG_NO_FORMATTING






class TestLog;

class Win32DateTimeTest
{
public:
    static void testLocales(TestLog *log);

private:
    Win32DateTimeTest();
};

#endif 

#endif 

#endif 
