





#ifndef _INTLTESTDATEFORMATSYMBOLS
#define _INTLTESTDATEFORMATSYMBOLS

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"




class IntlTestDateFormatSymbols: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    


    void TestSymbols();
    


    void TestGetMonths(void);
    void TestGetMonths2(void);

    void TestGetWeekdays2(void);
    void TestGetEraNames(void);
    void TestGetSetSpecificItems(void);

    UBool UnicodeStringsArePrefixes(int32_t count, int32_t prefixLen, const UnicodeString *prefixArray, const UnicodeString *baseArray);
};

#endif 

#endif
