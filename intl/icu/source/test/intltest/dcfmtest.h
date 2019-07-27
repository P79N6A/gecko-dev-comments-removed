













#ifndef DCFMTEST_H
#define DCFMTEST_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "intltest.h"


class DecimalFormatTest: public IntlTest {
public:

    DecimalFormatTest();
    virtual ~DecimalFormatTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    virtual void DataDrivenTests();

    
    virtual UChar *ReadAndConvertFile(const char *fileName, int32_t &len, UErrorCode &status);
    virtual const char *getPath(char buffer[2048], const char *filename);
    virtual void execParseTest(int32_t lineNum,
                              const UnicodeString &inputText,
                              const UnicodeString &expectedType,
                              const UnicodeString &expectedDecimal,
                              UErrorCode &status);

private:
    enum EFormatInputType {
        kFormattable,
        kStringPiece
    };

public:
    virtual void execFormatTest(int32_t lineNum,
                               const UnicodeString &pattern,
                               const UnicodeString &round, 
                               const UnicodeString &input,
                               const UnicodeString &expected,
                               EFormatInputType inType,
                               UErrorCode &status);
};

#endif
#endif
