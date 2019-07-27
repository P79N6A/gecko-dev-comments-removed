





#ifndef RESOURCEBUNDLETEST_H
#define RESOURCEBUNDLETEST_H

#include "intltest.h"




class ResourceBundleTest: public IntlTest {
public:
    ResourceBundleTest();
    virtual ~ResourceBundleTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestResourceBundles(void);
    


    void TestConstruction(void);

    void TestExemplar(void);

    void TestGetSize(void);
    void TestGetLocaleByType(void);

private:
    



    ResourceBundleTest& operator=(const ResourceBundleTest&) { return *this; }

    


    UBool testTag(const char* frag, UBool in_Root, UBool in_te, UBool in_te_IN);

    void record_pass(UnicodeString passMessage);
    void record_fail(UnicodeString errMessage);

    int32_t pass;
    int32_t fail;
};

#endif
