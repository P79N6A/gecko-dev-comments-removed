











#ifndef _JACOLL
#define _JACOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationKanaTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationKanaTest();
    virtual ~CollationKanaTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    void TestTertiary();

    
    void TestBase();

    
    void TestPlainDakutenHandakuten();

    
    void TestSmallLarge();

    
    void TestKatakanaHiragana();

    
    void TestChooonKigoo();

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];
    static const UChar testBaseCases[][MAX_TOKEN_LEN];
    static const UChar testPlainDakutenHandakutenCases[][MAX_TOKEN_LEN];
    static const UChar testSmallLargeCases[][MAX_TOKEN_LEN];
    static const UChar testKatakanaHiraganaCases[][MAX_TOKEN_LEN];
    static const UChar testChooonKigooCases[][MAX_TOKEN_LEN];

    Collator *myCollation;
};

#endif

#endif
