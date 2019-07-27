










#ifndef __WINNMTST
#define __WINNMTST

#include "unicode/utypes.h"

#if U_PLATFORM_USES_ONLY_WIN32_API

#if !UCONFIG_NO_FORMATTING






class TestLog;

class Win32NumberTest
{
public:
    static void testLocales(TestLog *log);

private:
    Win32NumberTest();
};

#endif 

#endif 

#endif 
