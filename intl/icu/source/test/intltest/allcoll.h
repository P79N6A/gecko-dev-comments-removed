











#ifndef _ALLCOLL
#define _ALLCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "tscoll.h"

class CollationDummyTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationDummyTest();
    virtual ~CollationDummyTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* );

    
    void TestPrimary();

    
    void TestSecondary();

    
    void TestTertiary();

    
    void TestExtra();

    void TestIdentical();

    void TestJB581();

private:
    static const  Collator::EComparisonResult results[];

    RuleBasedCollator *myCollation;
};

#endif

#endif
