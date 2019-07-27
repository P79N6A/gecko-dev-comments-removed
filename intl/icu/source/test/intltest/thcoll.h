









#ifndef COLLATIONTHAITEST_H
#define COLLATIONTHAITEST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationThaiTest : public IntlTestCollator {
    Collator* coll; 

public:

    CollationThaiTest();
    virtual ~CollationThaiTest();

    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
    
private:

    




    void TestDictionary(void);
    
    



    void TestCornerCases(void);
    
    




    void TestNamesList(void);

    


    void TestInvalidThai(void);

    


    void TestReordering(void);

private:

    void compareArray(Collator& c, const char* tests[],
                      int32_t testsLength);

    int8_t sign(int32_t i);
    
    




    UnicodeString& parseChars(UnicodeString& result,
                              const char* chars);
};

#endif 

#endif
