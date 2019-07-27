












#ifndef _ENCOLL
#define _ENCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationEnglishTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationEnglishTest();
    virtual ~CollationEnglishTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    void TestPrimary();

    
    void TestSecondary();

    
    void TestTertiary();

private:
    Collator *myCollation;
};

#endif

#endif
