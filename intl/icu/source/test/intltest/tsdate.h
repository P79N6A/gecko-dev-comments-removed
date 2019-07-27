





#ifndef _INTLTESTDATEFORMAT
#define _INTLTESTDATEFORMAT

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/datefmt.h"
#include "intltest.h"




class IntlTestDateFormat: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
    
private:

    


    void testAvailableLocales();
    


    void monsterTest();

    


    void testFormat();
    


    void tryDate(UDate date);
    


    void testLocale(const Locale&, const UnicodeString&);
    


    double randDouble(void);
    


    void describeTest(void);

    DateFormat *fFormat;
    UnicodeString fTestName;
    int32_t fLimit; 

    enum
    {
        
        ONESECOND = 1000,
        ONEMINUTE = 60 * ONESECOND,
        ONEHOUR = 60 * ONEMINUTE,
        ONEDAY = 24 * ONEHOUR
    };
    static const double ONEYEAR;
    enum EMode
    {
        GENERIC,
        TIME,
        DATE,
        DATE_TIME
    };
public:
    virtual ~IntlTestDateFormat();
};

#endif 

#endif
