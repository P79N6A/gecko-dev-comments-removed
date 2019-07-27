















#ifndef TRNSERR_H
#define TRNSERR_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"





class TransliteratorErrorTest : public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    
    void TestTransliteratorErrors(void);
    
    void TestUnicodeSetErrors(void);

    

    void TestRBTErrors(void);

    

    
    
    void TestCoverage(void);

};

#endif 

#endif
