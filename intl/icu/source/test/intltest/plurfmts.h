





#ifndef _PLURALFORMATTEST
#define _PLURALFORMATTEST

#include "unicode/utypes.h"
#include "unicode/plurrule.h"
#include "unicode/plurfmt.h"


#if !UCONFIG_NO_FORMATTING

#include "intltest.h"




class PluralFormatTest : public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    


    void pluralFormatBasicTest();
    void pluralFormatUnitTest();
    void pluralFormatLocaleTest();
    void pluralFormatExtendedTest();
    void pluralFormatExtendedParseTest();
    void ordinalFormatTest();
    void TestDecimals();
    void numberFormatTest(PluralFormat* plFmt, 
                          NumberFormat *numFmt, 
                          int32_t start, 
                          int32_t end, 
                          UnicodeString* numOddAppendStr,
                          UnicodeString* numEvenAppendStr, 
                          UBool overwrite, 
                          UnicodeString *message);
    void helperTestResults(const char** localeArray, 
                           int32_t capacityOfArray, 
                           UnicodeString& testPattern, 
                           int8_t *expectingResults);
};

#endif 

#endif
