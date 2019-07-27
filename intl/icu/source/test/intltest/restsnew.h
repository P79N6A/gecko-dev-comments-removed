





#ifndef NEW_RESOURCEBUNDLETEST_H
#define NEW_RESOURCEBUNDLETEST_H

#include "intltest.h"




class NewResourceBundleTest: public IntlTest {
public:
    NewResourceBundleTest();
    virtual ~NewResourceBundleTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestResourceBundles(void);
    


    void TestConstruction(void);

    void TestIteration(void);

    void TestOtherAPI(void);

    void TestNewTypes(void);

    void TestGetByFallback(void);

private:
    



    NewResourceBundleTest& operator=(const NewResourceBundleTest&) { return *this; }

    


    UBool testTag(const char* frag, UBool in_Root, UBool in_te, UBool in_te_IN);

    void record_pass(void);
    void record_fail(void);

    int32_t pass;
    int32_t fail;

};

#endif
