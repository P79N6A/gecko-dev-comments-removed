












#ifndef _MNKYTST
#define _MNKYTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationMonkeyTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationMonkeyTest();
    virtual ~CollationMonkeyTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    int32_t checkValue(int32_t value);

    
    void TestCompare();

    
    void TestCollationKey();

    void TestRules();

private:
    void report(UnicodeString& s, UnicodeString& t, int32_t result, int32_t revResult);

    const UnicodeString source;

    Collator *myCollator;
};

#endif

#endif
