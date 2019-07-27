





#ifndef _PluralRulesTest
#define _PluralRulesTest

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "unicode/localpointer.h"
#include "unicode/plurrule.h"




class PluralRulesTest : public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

private:
    


    void testAPI();
    void testGetUniqueKeywordValue();
    void testGetSamples();
    void testWithin();
    void testGetAllKeywordValues();
    void testOrdinal();
    void testSelect();
    void testAvailbleLocales();
    void testParseErrors();
    void testFixedDecimal();

    void assertRuleValue(const UnicodeString& rule, double expected);
    void assertRuleKeyValue(const UnicodeString& rule, const UnicodeString& key,
                            double expected);
    void checkSelect(const LocalPointer<PluralRules> &rules, UErrorCode &status, 
                                  int32_t line, const char *keyword, ...);
};

#endif 

#endif
