






#ifndef _PUTILTEST_
#define _PUTILTEST_

#include "intltest.h"




class PUtilTest : public IntlTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:


    void testMaxMin(void);

private:

    void maxMinTest(double a, double b, double exp, UBool max);

    
    void testNaN(void);
    void testPositiveInfinity(void);
    void testNegativeInfinity(void);
    void testZero(void);

    
    void testIsNaN(void);
    void NaNGT(void);
    void NaNLT(void);
    void NaNGTE(void);
    void NaNLTE(void);
    void NaNE(void);
    void NaNNE(void);

};
 
#endif

