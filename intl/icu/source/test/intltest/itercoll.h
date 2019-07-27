










#ifndef _ITERCOLL
#define _ITERCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "tscoll.h"

class CollationIteratorTest: public IntlTestCollator
{
public:

    
    
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationIteratorTest();
    virtual ~CollationIteratorTest();

    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL);

    




    void TestUnicodeChar();

    





    void TestPrevious();
    
    


    void TestOffset();

    


    void TestSetText();
    
    


    void TestMaxExpansion();

    


    void TestClearBuffers();

    


    void TestAssignment();

    


    void TestConstructors();

    


    void TestStrengthOrder();
    
    
    
    

private:

    struct ExpansionRecord
    {
        UChar character;
        int32_t count;
    };

    


    void verifyExpansion(UnicodeString rules, ExpansionRecord tests[], int32_t testCount);
    
    



    UnicodeString &orderString(CollationElementIterator &iter, UnicodeString &target);

    void assertEqual(CollationElementIterator &i1, CollationElementIterator &i2);

    RuleBasedCollator *en_us;
    const UnicodeString test1;
    const UnicodeString test2;

};

#endif

#endif
