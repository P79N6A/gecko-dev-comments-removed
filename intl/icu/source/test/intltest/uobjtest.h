






#ifndef _UOBJECTTEST_
#define _UOBJECTTEST_

#include "intltest.h"




class UObjectTest : public IntlTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
private:
    
    void testIDs();
    void testUMemory();
    void TestMFCCompatibility();
    void TestCompilerRTTI();

    

    






    UObject *testClass(UObject *obj,
               const char *className, const char *factory, 
               UClassID staticID);

    UObject *testClassNoClassID(UObject *obj,
               const char *className, const char *factory);
};

#endif

