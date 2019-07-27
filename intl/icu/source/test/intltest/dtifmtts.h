





#ifndef _INTLTESTDATEINTERVALFORMAT
#define _INTLTESTDATEINTERVALFORMAT

#include "unicode/utypes.h"
#include "unicode/locid.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"




class DateIntervalFormatTest: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

public:
    


    void testAPI();

    


    void testFormat();

    


    void testFormatUserDII();

    



    void testSetIntervalPatternNoSideEffect();

    


    void testYearFormats();

    


    void testStress();

    void testTicket11583_2();

private:
    


    void expect(const char** data, int32_t data_length);

    



    void expectUserDII(const char** data, int32_t data_length);

    


    void stress(const char** data, int32_t data_length, const Locale& loc, 
                const char* locName);
};

#endif 

#endif
