



















#ifndef __STRTEST_H__
#define __STRTEST_H__

#include "intltest.h"

class StringTest : public IntlTest {
public:
    StringTest() {}
    virtual ~StringTest();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

private:
    void TestEndian();
    void TestSizeofTypes();
    void TestCharsetFamily();
    void Test_U_STRING();
    void Test_UNICODE_STRING();
    void Test_UNICODE_STRING_SIMPLE();
    void Test_UTF8_COUNT_TRAIL_BYTES();
    void TestStringPiece();
    void TestStringPieceComparisons();
    void TestByteSink();
    void TestCheckedArrayByteSink();
    void TestStringByteSink();
    void TestSTLCompatibility();
    void TestCharString();
};

#endif
