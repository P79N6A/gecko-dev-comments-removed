





#ifndef __IntlCalendarTest__
#define __IntlCalendarTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "caltztst.h"

class IntlCalendarTest: public CalendarTimeZoneTest {
public:
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    void TestTypes(void);

    void TestGregorian(void);

    void TestBuddhist(void);
    void TestBuddhistFormat(void);

    void TestTaiwan(void);

    void TestJapanese(void);
    void TestJapaneseFormat(void);
    void TestJapanese3860(void);
    
    void TestPersian(void);
    void TestPersianFormat(void);

 protected:
    
    void quasiGregorianTest(Calendar& cal, const Locale& gregoLocale, const int32_t *data);
    void simpleTest(const Locale& loc, const UnicodeString& expect, UDate expectDate, UErrorCode& status);
 
public: 
    
    static UnicodeString value(Calendar* calendar);
 
};


#endif 
 
#endif 
