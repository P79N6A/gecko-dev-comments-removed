




 
#ifndef __CalendarLimitTest__
#define __CalendarLimitTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "caltztst.h"







class CalendarLimitTest: public CalendarTimeZoneTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public: 
    
    virtual void test(UDate millis, Calendar *cal, DateFormat *fmt);

    
    
    
    static UBool withinErr(double a, double b, double err);

public:
    
    virtual void TestCalendarExtremeLimit(void);

    void TestLimits(void);

private:
    





    void doTheoreticalLimitsTest(Calendar& cal, UBool leapMonth);

    


























    void doLimitsTest(Calendar& cal, const int32_t* fieldsToTest, UDate startDate, int32_t testDuration);

    


    void doLimitsTest(Calendar& cal, UDate startDate, int32_t endTime);

    UnicodeString& ymdToString(const Calendar& cal, UnicodeString& str);
};

#endif 
 
#endif 
