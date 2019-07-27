





#ifndef UNICODESTRINGTEST_H
#define UNICODESTRINGTEST_H

#include "unicode/unistr.h"
#include "intltest.h"

U_NAMESPACE_BEGIN

class Appendable;

U_NAMESPACE_END




class UnicodeStringTest: public IntlTest {
public:
    UnicodeStringTest() {}
    virtual ~UnicodeStringTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestBasicManipulation(void);
    


    void TestCompare(void);
    


    void TestExtract(void);
    


    void TestRemoveReplace(void);
    


    void TestSearching(void);
    


    void TestSpacePadding(void);
    


    void TestPrefixAndSuffix(void);
    void TestStartsWithAndEndsWithNulTerminated();
    


    void TestFindAndReplace(void);
    


    void TestReverse(void);
    


    void TestMiscellaneous(void);
    


    void TestStackAllocation(void);
    


    void TestUnescape(void);

    void _testUnicodeStringHasMoreChar32Than(const UnicodeString &s, int32_t start, int32_t length, int32_t number);
    void TestCountChar32();
    void TestBogus();
    void TestStringEnumeration();
    void TestNameSpace();
    void TestUTF32();
    void TestUTF8();
    void TestReadOnlyAlias();
    void doTestAppendable(UnicodeString &dest, Appendable &app);
    void TestAppendable();
    void TestUnicodeStringImplementsAppendable();
    void TestSizeofUnicodeString();
};

class StringCaseTest: public IntlTest {
public:
    StringCaseTest() {}
    virtual ~StringCaseTest();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=0);

    void TestCaseConversion();

    void TestCasingImpl(const UnicodeString &input,
                        const UnicodeString &output,
                        int32_t whichCase,
                        void *iter, const char *localeID, uint32_t options);
    void TestCasing();
    void TestFullCaseFoldingIterator();
};

#endif
