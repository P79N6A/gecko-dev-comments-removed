









#ifndef UVECTEST_H
#define UVECTEST_H

#include "unicode/utypes.h"

#include "intltest.h"


class UVectorTest: public IntlTest {
public:
  
    UVectorTest();
    virtual ~UVectorTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    void UVector_API();
    void UStack_API();
    void Hashtable_API();

};

#endif
