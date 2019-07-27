












#ifndef _TESTUNISET
#define _TESTUNISET

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/ucnv_err.h"
#include "intltest.h"

class UnicodeSetWithStrings;




class UnicodeSetTest: public IntlTest {
public:
    UnicodeSetTest();
    ~UnicodeSetTest();

private:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    void Testj2268();

    



    void TestToPattern();
    
    void TestPatterns(void);
    void TestCategories(void);
    void TestAddRemove(void);
    void TestCloneEqualHash(void);

    


    void TestMinimalRep(void);

    void TestAPI(void);

    void TestIteration(void);

    void TestStrings(void);

    void TestScriptSet(void);

    


    void TestPropertySet(void);

    void TestClone(void);

    void TestIndexOf(void);

    void TestExhaustive(void);

    void TestCloseOver(void);

    void TestEscapePattern(void);

    void TestInvalidCodePoint(void);

    void TestSymbolTable(void);

    void TestSurrogate();

    void TestPosixClasses();

    void TestFreezable();

    void TestSpan();

    void TestStringSpan();

private:

    UBool toPatternAux(UChar32 start, UChar32 end);
    
    UBool checkPat(const UnicodeString& source,
                   const UnicodeSet& testSet);

    UBool checkPat(const UnicodeString& source, const UnicodeSet& testSet, const UnicodeString& pat);

    void _testComplement(int32_t a, UnicodeSet&, UnicodeSet&);

    void _testAdd(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    void _testRetain(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    void _testRemove(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    void _testXor(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    



    void checkCanonicalRep(const UnicodeSet& set, const UnicodeString& msg);

    


    static UnicodeSet& bitsToSet(int32_t a, UnicodeSet&);

    



    static int32_t setToBits(const UnicodeSet& x);

    




    static UnicodeString getPairs(const UnicodeSet& set);

    




    void checkRoundTrip(const UnicodeSet& s);
    
    void copyWithIterator(UnicodeSet& t, const UnicodeSet& s, UBool withRange);
    
    UBool checkEqual(const UnicodeSet& s, const UnicodeSet& t, const char* message);

    void expectContainment(const UnicodeString& pat,
                           const UnicodeString& charsIn,
                           const UnicodeString& charsOut);
    void expectContainment(const UnicodeSet& set,
                           const UnicodeString& charsIn,
                           const UnicodeString& charsOut);
    void expectContainment(const UnicodeSet& set,
                           const UnicodeString& setName,
                           const UnicodeString& charsIn,
                           const UnicodeString& charsOut);
    void expectPattern(UnicodeSet& set,
                       const UnicodeString& pattern,
                       const UnicodeString& expectedPairs);
    void expectPairs(const UnicodeSet& set,
                     const UnicodeString& expectedPairs);
    void expectToPattern(const UnicodeSet& set,
                         const UnicodeString& expPat,
                         const char** expStrings);
    void expectRange(const UnicodeString& label,
                     const UnicodeSet& set,
                     UChar32 start, UChar32 end);
    void doAssert(UBool, const char*);

    void testSpan(const UnicodeSetWithStrings *sets[4], const void *s, int32_t length, UBool isUTF16,
                  uint32_t whichSpans,
                  int32_t expectLimits[], int32_t &expectCount,
                  const char *testName, int32_t index);
    void testSpan(const UnicodeSetWithStrings *sets[4], const void *s, int32_t length, UBool isUTF16,
                  uint32_t whichSpans,
                  const char *testName, int32_t index);
    void testSpanBothUTFs(const UnicodeSetWithStrings *sets[4],
                          const UChar *s16, int32_t length16,
                          uint32_t whichSpans,
                          const char *testName, int32_t index);
    void testSpanContents(const UnicodeSetWithStrings *sets[4], uint32_t whichSpans, const char *testName);
    void testSpanUTF16String(const UnicodeSetWithStrings *sets[4], uint32_t whichSpans, const char *testName);
    void testSpanUTF8String(const UnicodeSetWithStrings *sets[4], uint32_t whichSpans, const char *testName);

    UConverter *openUTF8Converter();

    UConverter *utf8Cnv;

public:
    static UnicodeString escape(const UnicodeString& s);
};

#endif
