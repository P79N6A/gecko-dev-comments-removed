






#ifndef _SELECTFORMATTEST
#define _SELECTFORMATTEST

#include "unicode/utypes.h"
#include "unicode/selfmt.h"


#if !UCONFIG_NO_FORMATTING

#include "intltest.h"




class SelectFormatTest : public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    


    void selectFormatAPITest();
    void selectFormatUnitTest();
};

#endif 

#endif
