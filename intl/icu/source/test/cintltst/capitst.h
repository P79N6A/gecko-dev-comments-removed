














#ifndef _CCOLLAPITST
#define _CCOLLAPITST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cintltst.h"
#include "callcoll.h"
#define MAX_TOKEN_LEN 16


    



    static void doAssert(int condition, const char *message);
    




    void TestProperty(void);
    


    void TestRuleBasedColl(void);
    
    


    void TestCompare(void);
    


    void TestHashCode(void);
    


   void TestSortKey(void);
    


   void TestElemIter(void);
    


    void TestGetAll(void);
    




    void TestDecomposition(void);
    

    
    void TestSafeClone(void);

    


    void TestCloneBinary(void);

    


    void TestOpenVsOpenRules(void);

    


    void TestBounds(void);

    


    void TestGetLocale(void);

    


    void TestSortKeyBufferOverrun(void);
    


    void TestGetSetAttr(void);
    


    void TestGetTailoredSet(void);

    


    void TestMergeSortKeys(void);

    


    static void TestShortString(void);

    


    static void TestGetContractionsAndUnsafes(void);

    


    static void TestOpenBinary(void);

    


    static void TestGetKeywordValuesForLocale(void);

    


    static void TestStrcollNull(void);

#endif 

#endif
