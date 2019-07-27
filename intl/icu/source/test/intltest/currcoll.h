










#ifndef _CURRCOLL
#define _CURRCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coleitr.h"
#include "tscoll.h"

class CollationCurrencyTest: public IntlTestCollator
{
public:

    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationCurrencyTest();
    virtual ~CollationCurrencyTest();
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL);

    void currencyTest();
};

#endif

#endif
