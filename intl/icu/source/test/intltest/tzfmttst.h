






#ifndef _TIMEZONEFORMATTEST_
#define _TIMEZONEFORMATTEST_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

class TimeZoneFormatTest : public IntlTest {
    
    void runIndexedTest(int32_t index, UBool exec, const char*& name, char* par);

    void TestTimeZoneRoundTrip(void);
    void TestTimeRoundTrip(void);
    void TestParse(void);
    void TestISOFormat(void);
    void TestFormat(void);
    void TestFormatTZDBNames(void);
};

#endif 
 
#endif 
