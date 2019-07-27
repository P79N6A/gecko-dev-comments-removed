





#ifndef _INTLTESTNUMBERFORMAT
#define _INTLTESTNUMBERFORMAT


#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "intltest.h"






class IntlTestNumberFormat: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:

    


    void testFormat();
    


    void tryIt(double aNumber);
    


    void tryIt(int32_t aNumber);
    


    void testAvailableLocales();
    


    void monsterTest();
    


    void testLocale(const Locale& locale, const UnicodeString& localeName);

    NumberFormat*   fFormat;
    UErrorCode      fStatus;
    Locale          fLocale;

public:

    virtual ~IntlTestNumberFormat();

    


    static double getSafeDouble(double smallerThanMax);

    


    static double randDouble();

    


    static uint32_t randLong();

    


    static double randFraction()
    {
        return (double)randLong() / (double)0xFFFFFFFF;
    }

};

#endif 

#endif
