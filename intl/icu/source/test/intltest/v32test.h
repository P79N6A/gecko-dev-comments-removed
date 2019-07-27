









#ifndef V32TEST_H
#define V32TEST_H

#include "unicode/utypes.h"

#include "intltest.h"


class UVector32Test: public IntlTest {
public:
  
    UVector32Test();
    virtual ~UVector32Test();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    
    virtual void UVector32_API();

};

#endif
