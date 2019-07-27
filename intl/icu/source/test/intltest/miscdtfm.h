





#ifndef _DATEFORMATMISCTEST_
#define _DATEFORMATMISCTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"




class DateFormatMiscTests : public IntlTest {    
    
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

    void test4097450(void);
    void test4099975(void);
    void test4117335(void);

protected:
    UBool failure(UErrorCode status, const char* msg);

};

#endif 
 
#endif 

