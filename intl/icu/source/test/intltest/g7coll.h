





























#ifndef _G7COLL
#define _G7COLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "tscoll.h"

class G7CollationTest: public IntlTestCollator {
public:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    enum ETotal_Locales { TESTLOCALES = 12 };
    enum ETotal_Fixed { FIXEDTESTSET = 15 };
    enum ETotal_Test { TOTALTESTSET = 30 };

    G7CollationTest() {}
    virtual ~G7CollationTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );


    
    void TestG7Locales();

    
    void TestDemo1();

    
    void TestDemo2();

    
    
    void TestDemo3();

    
    
    void TestDemo4();

};

#endif

#endif
