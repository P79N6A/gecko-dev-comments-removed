









#ifndef RBBITEST_H
#define RBBITEST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "intltest.h"
#include "unicode/brkiter.h"


class  Enumeration;
class  BITestData;
struct TestParams;
class  RBBIMonkeyKind;

U_NAMESPACE_BEGIN
class  UVector32;
U_NAMESPACE_END




class RBBITest: public IntlTest {
public:

    RBBITest();
    virtual ~RBBITest();

    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestStatusReturn();

    void TestEmptyString();
    void TestGetAvailableLocales();
    void TestGetDisplayName();
    void TestEndBehaviour();
    void TestBug4153072();
    void TestJapaneseLineBreak();
    void TestThaiLineBreak();
    void TestMixedThaiLineBreak();
    void TestMaiyamok();
    void TestMonkey(char *params);

    void TestExtended();
    UChar *ReadAndConvertFile(const char *fileName, int &ulen, const char *encoding, UErrorCode &status);
    void executeTest(TestParams *, UErrorCode &status);

    void TestWordBreaks();
    void TestWordBoundary();
    void TestLineBreaks();
    void TestSentBreaks();
    void TestBug3818();
    void TestJapaneseWordBreak();
    void TestTrieDict();
    void TestUnicodeFiles();
    void TestBug5775();
    void TestTailoredBreaks();
    void TestDictRules();
    void TestBug5532();
    void TestBug9983();

    void TestDebug();
    void TestProperties();


private:
    



    






    void generalIteratorTest(RuleBasedBreakIterator& bi, BITestData  &td);
    


    void testFirstAndNext(RuleBasedBreakIterator& bi, BITestData &td);
    


    void testLastAndPrevious(RuleBasedBreakIterator& bi, BITestData &td);
    


    void testFollowing(RuleBasedBreakIterator& bi, BITestData &td);
    


    void testPreceding(RuleBasedBreakIterator& bi, BITestData &td);
    


    void testIsBoundary(RuleBasedBreakIterator& bi, BITestData &td);
    



    void doMultipleSelectionTest(RuleBasedBreakIterator& iterator, BITestData &td);

    void RunMonkey(BreakIterator *bi, RBBIMonkeyKind &mk, const char *name, uint32_t  seed,
        int32_t loopCount, UBool useUText);

    
    void runUnicodeTestData(const char *fileName, RuleBasedBreakIterator *bi);

    
    void checkUnicodeTestCase(const char *testFileName, int lineNumber,
                         const UnicodeString &testString,
                         UVector32 *breakPositions,
                         RuleBasedBreakIterator *bi);

    
    void TBTest(BreakIterator* brkitr, int type, const char *locale, const char* escapedText,
                const int32_t *expectOffsets, int32_t expectOffsetsCount);

    






    UBool testCaseIsKnownIssue(const UnicodeString &testCase, const char *fileName);
};

#endif 

#endif
