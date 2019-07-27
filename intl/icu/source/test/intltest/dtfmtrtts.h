





#ifndef _DATEFORMATROUNDTRIPTEST_
#define _DATEFORMATROUNDTRIPTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/calendar.h"
#include "intltest.h"




class DateFormatRoundTripTest : public IntlTest {    
    
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );

public:
    DateFormatRoundTripTest();
    virtual ~DateFormatRoundTripTest();
    
    void TestDateFormatRoundTrip(void);
    void TestCentury(void);
    void test(const Locale& loc);
    void test(DateFormat *fmt, const Locale &origLocale, UBool timeOnly = FALSE );
    int32_t getField(UDate d, int32_t f);
    UnicodeString& escape(const UnicodeString& src, UnicodeString& dst);
    UDate generateDate(void); 
    UDate generateDate(UDate minDate); 









static uint32_t randLong() {
    
    
    return ((uint32_t) (IntlTest::random() * (1<<16))) |
          (((uint32_t) (IntlTest::random() * (1<<16))) << 16);
}




static double randFraction()
{
    return (double)randLong() / (double)0xFFFFFFFF;
}




static double randDouble(double range)
{
    double a = randFraction();
    
    
    return (2.0 * range * a) - range;
}

protected:
    UBool failure(UErrorCode status, const char* msg);
    UBool failure(UErrorCode status, const char* msg, const UnicodeString& str);

    const UnicodeString& fullFormat(UDate d);

private:

    static int32_t SPARSENESS;
    static int32_t TRIALS;
    static int32_t DEPTH;

    UBool optionv; 
    SimpleDateFormat *dateFormat;
    UnicodeString fgStr;
    Calendar *getFieldCal;
};

#endif 
 
#endif 

