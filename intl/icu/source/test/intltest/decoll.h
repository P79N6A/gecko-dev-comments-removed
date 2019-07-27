










#ifndef _DECOLL
#define _DECOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationGermanTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationGermanTest();
    virtual ~CollationGermanTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    void TestPrimary();

    
    void TestSecondary();

    
    void TestTertiary();

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[][2];

    Collator *myCollation;
};

#endif

#endif
