











#ifndef _TRCOLL
#define _TRCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationTurkishTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationTurkishTest();
    virtual ~CollationTurkishTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    void TestPrimary();

    
    void TestTertiary();

private:
    
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];

    Collator *myCollation;
};

#endif

#endif
