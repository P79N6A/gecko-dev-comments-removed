













#ifndef _CRESTST
#define _CRESTST

#include "cintltst.h"




    void addTestResourceBundleTest(TestNode**);

    


    void TestResourceBundles(void);
    


    void TestConstruction1(void);

    void TestConstruction2(void);

    void TestAliasConflict(void);

    static void TestGetSize(void);

    static void TestGetLocaleByType(void);

    



    UBool testTag(const char* frag, UBool in_Root, UBool in_te, UBool in_te_IN);

    void record_pass(void);
    void record_fail(void);


    int32_t pass;
    int32_t fail;

#endif
