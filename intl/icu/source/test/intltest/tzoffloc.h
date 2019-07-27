






#ifndef _TIMEZONEOFFSETLOCALTEST_
#define _TIMEZONEOFFSETLOCALTEST_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

class TimeZoneOffsetLocalTest : public IntlTest {
    
    void runIndexedTest(int32_t index, UBool exec, const char*& name, char* par);

    void TestGetOffsetAroundTransition(void);
};

#endif 
 
#endif 
