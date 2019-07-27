











#ifndef RBBIAPITEST_H
#define RBBIAPITEST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "intltest.h"
#include "unicode/rbbi.h"




class RBBIAPITest: public IntlTest {
public:
   
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
    


   
    


    void TestCloneEquals();
    


    void TestgetRules();
    


    void TestHashCode();
     


    void TestGetSetAdoptText();
     


    void TestIteration(void);

    void TestFilteredBreakIteratorBuilder(void);

    


    void TestBuilder(void);

    void TestRoundtripRules(void);

    void RoundtripRule(const char *dataFile);

    



    void TestCreateFromRBBIData(void);

    


    void TestQuoteGrouping();

    


    void TestRuleStatus();
    void TestRuleStatusVec();

    void TestBug2190();

    void TestBoilerPlate();

    void TestRegistration();

    void TestRefreshInputText();

    


     
    void doBoundaryTest(BreakIterator& bi, UnicodeString& text, int32_t *boundaries);

    
    void doTest(UnicodeString& testString, int32_t start, int32_t gotoffset, int32_t expectedOffset, const char* expected);


};




class RBBIWithProtectedFunctions: public RuleBasedBreakIterator {
public:
    enum EDontAdopt {
        kDontAdopt
    };
    RBBIWithProtectedFunctions(RBBIDataHeader* data, UErrorCode &status);
    RBBIWithProtectedFunctions(const RBBIDataHeader* data, enum EDontAdopt dontAdopt, UErrorCode &status);
};

#endif

#endif
