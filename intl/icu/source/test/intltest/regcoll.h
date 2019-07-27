










#ifndef _REGCOLL
#define _REGCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coleitr.h"
#include "tscoll.h"

class CollationRegressionTest: public IntlTestCollator
{
public:

    
    
    enum EToken_Len { MAX_TOKEN_LEN = 32 };

    CollationRegressionTest();
    virtual ~CollationRegressionTest();

    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    
    
    
    void Test4048446();

    
    
    
    
    void Test4051866();

    
    
    
    
    void Test4053636();


    
    
    
    
    
    void Test4054238();

    
    
    
    
    void Test4054734();

    
    
    
    
    void Test4054736();

    
    
    
    
    void Test4058613();
    
    
    
    
    
    
    void Test4059820();

    
    
    
    
    void Test4060154();

    
    
    
    
    void Test4062418();

    
    
    
    
    void Test4065540();

    
    
    
    
    
    
    void Test4066189();

    
    
    
    
    void Test4066696();


    
    
    
    
    void Test4076676();


    
    
    
    
    void Test4078588();

    
    
    
    
    void Test4079231();

    
    
    
    
    void Test4081866();

    
    
    
    
    void Test4087241();

    
    
    
    
    void Test4087243();

    
    
    
    
    
    void Test4092260();

    
    
    void Test4095316();

    
    
    void Test4101940();

    
    
    
    
    void Test4103436();

    
    
    
    
    void Test4114076();
    
    
    
    
    
    
    void Test4114077();

    
    
    
    
    void Test4124632();
    
    
    
    
    
    void Test4132736();
    
    
    
    
    
    void Test4133509();

    
    
    
    
    
    void Test4139572();
    
    
    
    
    
    void Test4141640();
    
    
    
    
    
    void Test4146160();

    void Test4179216();

    
    
    
    
    void TestT7189();

    
    
    
    
    void TestCaseFirstCompression();

    void TestTrailingComment();
    void TestBeforeWithTooStrongAfter();

private:
    
    
    
    void compareArray(Collator &c,
                    const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN],
                    int32_t testCount);

    void assertEqual(CollationElementIterator &i1, CollationElementIterator &i2);


    RuleBasedCollator *en_us;

    void caseFirstCompressionSub(Collator *col, UnicodeString opt);
};

#endif

#endif
