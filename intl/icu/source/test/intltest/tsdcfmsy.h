





#ifndef _INTLTESTDECIMALFORMATSYMBOLS
#define _INTLTESTDECIMALFORMATSYMBOLS

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/dcfmtsym.h"
#include "intltest.h"



class IntlTestDecimalFormatSymbols: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    


    void testSymbols();
    void testLastResortData();

     
    void Verify(double value, const UnicodeString& pattern,
                const DecimalFormatSymbols &sym, const UnicodeString& expected);
};

#endif 

#endif
