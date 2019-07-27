










#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"





class TestChoiceFormat: public IntlTest {
    


    void TestSimpleExample(void); 
    




    void TestComplexExample(void);

    


    void TestClosures(void);

    


    void TestPatterns(void);
    void TestChoiceFormatToPatternOverflow(void);

    void _testPattern(const char* pattern,
                      UBool isValid,
                      double v1, const char* str1,
                      double v2, const char* str2,
                      double v3, const char* str3);
    


    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

#endif 
