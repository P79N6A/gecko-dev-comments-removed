








#ifndef TRANSTST_H
#define TRANSTST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"





class TransliteratorTest : public IntlTest {

public:
    TransliteratorTest();
    virtual ~TransliteratorTest();

private:
    void runIndexedTest(int32_t index, UBool exec, const char* &name,
                        char* par=NULL);

    void TestInstantiation(void);
    
    void TestSimpleRules(void);

    void TestInlineSet(void);

    void TestAnchors(void);

    void TestPatternQuoting(void);

    







    void TestRuleBasedInverse(void);

    


    void TestKeyboard(void);

    


    void TestKeyboard2(void);

    


    void TestKeyboard3(void);
    
    void keyboardAux(const Transliterator& t,
                     const char* DATA[], int32_t DATA_length);
    
    void TestArabic(void);

    



    void TestCompoundKana(void);

    


    void TestCompoundHex(void);

    


    void TestFiltering(void);

    


    void TestJ277(void);

    


    void TestJ243(void);

    


    void TestJ329(void);

    


    void TestSegments(void);
    
    


    void TestCursorOffset(void);
    
    



    void TestArbitraryVariableValues(void);

    



    void TestPositionHandling(void);

    


    void TestHiraganaKatakana(void);

    


    void TestCopyJ476(void);

    



    void TestInterIndic(void);

    


    void TestFilterIDs(void);

    


    void TestCaseMap(void);

    


    void TestNameMap(void);

    


    void TestLiberalizedID(void);
    


    void TestCreateInstance(void);

    void TestNormalizationTransliterator(void);

    void TestCompoundRBT(void);

    void TestCompoundFilter(void);

    void TestRemove(void);

    void TestToRules(void);

    void TestContext(void);

    void TestSupplemental(void);

    void TestQuantifier(void);

    


    void TestSTV(void);

    void TestCompoundInverse(void);

    void TestNFDChainRBT(void);

    


    void TestNullInverse(void);
    
    


    void TestAliasInverseID(void);
    
    


    void TestCompoundInverseID(void);
    
    


    void TestUndefinedVariable(void);
    
    


    void TestEmptyContext(void);

    


    void TestCompoundFilterID(void);

    


    void TestPropertySet(void);

    


    void TestNewEngine(void);

    



    void TestQuantifiedSegment(void);

    
    void TestDevanagariLatinRT(void);

    
    void TestTeluguLatinRT(void);
    
    
    void TestGujaratiLatinRT(void);
    
    
    void TestSanskritLatinRT(void);
    
    
    void TestCompoundLatinRT(void);

    
    void TestGurmukhiDevanagari(void);
    


    void TestLocaleInstantiation(void);        
    
    


    void TestTitleAccents(void);

    


    void TestLocaleResource(void);

    


    void TestParseError(void);

    


    void TestOutputSet(void);

    



    void TestVariableRange(void);

    


    void TestInvalidPostContext(void);

    


    void TestIDForms(void);

    


    void TestToRulesMark(void);

    


    void TestEscape(void);

    void TestAnchorMasking(void);

    


    void TestDisplayName(void);
    
    


    void TestSpecialCases(void);
    


    void TestIncrementalProgress(void);

    


    void TestSurrogateCasing (void);

    void TestFunction(void);

    void TestInvalidBackRef(void);

    void TestMulticharStringSet(void);

    void TestUserFunction(void);

    void TestAnyX(void);

    void TestAny(void);

    void TestSourceTargetSet(void);

    void TestPatternWhiteSpace(void);

    void TestAllCodepoints(void);

    void TestBoilerplate(void);

    void TestAlternateSyntax(void);

    void TestRuleStripping(void);

    void TestHalfwidthFullwidth(void);

    void TestThai(void);

    


    void TestBeginEnd(void);

    


    void TestBeginEndToRules(void);

    


    void TestRegisterAlias(void);

    
    
    
 protected:
    void expectT(const UnicodeString& id,
                 const UnicodeString& source,
                 const UnicodeString& expectedResult);

    void expect(const UnicodeString& rules,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                UTransPosition *pos=0);

    void expect(const UnicodeString& id,
                const UnicodeString& rules,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                UTransPosition *pos=0);

    void expect(const Transliterator& t,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                const Transliterator& reverseTransliterator);
    
    void expect(const Transliterator& t,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                UTransPosition *pos=0);
    
    void expectAux(const UnicodeString& tag,
                   const UnicodeString& source,
                   const UnicodeString& result,
                   const UnicodeString& expectedResult);
    
    virtual void expectAux(const UnicodeString& tag,
                   const UnicodeString& summary, UBool pass,
                   const UnicodeString& expectedResult);

    static UnicodeString& formatInput(UnicodeString &appendTo,
                                      const UnicodeString& input,
                                      const UTransPosition& pos);

    void checkRules(const UnicodeString& label, Transliterator& t2,
                    const UnicodeString& testRulesForward);
    void CheckIncrementalAux(const Transliterator* t, 
                             const UnicodeString& input);

    void reportParseError(const UnicodeString& message, const UParseError& parseError, const UErrorCode& status);


    const UnicodeString DESERET_DEE;
    const UnicodeString DESERET_dee;

};

#endif 

#endif
