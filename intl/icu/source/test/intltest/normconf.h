






#ifndef _NORMCONF
#define _NORMCONF

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/normlzr.h"
#include "intltest.h"

typedef struct _FileStream FileStream;

class NormalizerConformanceTest : public IntlTest {
    Normalizer normalizer;

 public:
    NormalizerConformanceTest();
    virtual ~NormalizerConformanceTest();

    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    



    void TestConformance();
    void TestConformance32();
    void TestConformance(FileStream *input, int32_t options);

    
    
    void TestCase6(void);

 private:
    FileStream *openNormalizationTestFile(const char *filename);

    














    UBool checkConformance(const UnicodeString* field,
                           const char *line,
                           int32_t options,
                           UErrorCode &status);

    void iterativeNorm(const UnicodeString& str,
                       UNormalizationMode mode, int32_t options,
                       UnicodeString& result,
                       int8_t dir);

    







    UBool assertEqual(const char *op,
                      const UnicodeString& s,
                      const UnicodeString& got,
                      const UnicodeString& exp,
                      const char *msg,
                      int32_t field);

    











    UBool hexsplit(const char *s, char delimiter,
                   UnicodeString output[], int32_t outputLength);

    void _testOneLine(const char *line);
    void compare(const UnicodeString& s1,const UnicodeString& s2);
};

#endif 

#endif
