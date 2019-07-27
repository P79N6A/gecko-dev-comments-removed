





#ifndef CHARITERTEST_H
#define CHARITERTEST_H

#include "intltest.h"
#include "unicode/uiter.h"




class CharIterTest: public IntlTest {
public:
    CharIterTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestConstructionAndEquality(void);
    


    void TestConstructionAndEqualityUChariter(void);
    


    void TestIteration(void);
     


    void TestIterationUChar32(void);

    void TestUCharIterator();
    void TestUCharIterator(UCharIterator *iter, CharacterIterator &ci, const char *moves, const char *which);
    void TestCoverage();
    void TestCharIteratorSubClasses();
};

#endif

