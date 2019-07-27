






#ifndef _INTLTESTDECIMALFORMATAPI
#define _INTLTESTDECIMALFORMATAPI

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "intltest.h"


class IntlTestDecimalFormatAPI: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

public:
    


    void testAPI();
    void testRounding();
    void testRoundingInc();
    void TestCurrencyPluralInfo();
    void TestScale();
    void TestFixedDecimal();
    void TestBadFastpath();
    void TestRequiredDecimalPoint();
private:
    
    void verify(const UnicodeString& message, const UnicodeString& got, double expected);
    void verifyString(const UnicodeString& message, const UnicodeString& got, UnicodeString& expected);
};

#endif 

#endif
