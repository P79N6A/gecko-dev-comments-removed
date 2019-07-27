










#ifndef _FRCOLL
#define _FRCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationFrenchTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationFrenchTest();
    virtual ~CollationFrenchTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    void TestSecondary();

    
    void TestTertiary();

    
    void TestExtra();

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const UChar testBugs[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];
    static const UChar testAcute[][MAX_TOKEN_LEN];

    Collator *myCollation;
};

#endif

#endif
