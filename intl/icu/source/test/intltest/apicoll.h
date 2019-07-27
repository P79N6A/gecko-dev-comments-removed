










#ifndef _APICOLL
#define _APICOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "tscoll.h"

class CollationAPITest: public IntlTestCollator {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* );
    void doAssert(UBool condition, const char *message);

    







    void TestProperty();

    




    void TestRuleBasedColl();

    



    void TestRules();

    


    void TestDecomposition();

    


    void TestSafeClone();

    






    void TestOperators();

    


    void TestDuplicate();

    





    void TestCompare();

    


    void TestHashCode();

    







    void TestCollationKey();

    







    void TestElemIter();

    


    void TestGetAll();

    


    void TestSortKey();
    void TestSortKeyOverflow();

    


    void TestMaxExpansion();

    


    void TestDisplayName();

    


    void TestAttribute();

    


    void TestVariableTopSetting();
    void TestMaxVariable();

    


    void TestGetLocale();

    


    void TestBounds();

    


    void TestGetTailoredSet();

    


    void TestSubclass();

    


    void TestUClassID();

    


    void TestNULLCharTailoring();

    void TestClone();
    void TestCloneBinary();
    void TestIterNumeric();
    void TestBadKeywords();

private:
    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    void dump(UnicodeString msg, RuleBasedCollator* c, UErrorCode& status);

};

#endif

#endif
