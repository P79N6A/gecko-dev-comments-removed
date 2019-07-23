



































#ifndef OJITestLoader_h___
#define OJITestLoader_h___

#include "nsISupports.h"

#include "ojiapitests.h"
#include "testtypes.h"

#include <sys/stat.h>
#include <string.h>

#define OJITESTLOADER_IID                            \
{ /* a1e5ed51-aa4a-11d1-85b2-00805f0e4dfe */         \
    0xa1e5ed51,                                      \
    0xaa4a,                                          \
    0x11d1,                                          \
    {0x85, 0xb2, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

#define OJITESTLOADER_CID                            \
{ /* 38e7ef11-58df-11d2-8164-006008119d7a */         \
    0x38e7ef11,                                      \
    0x58df,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}


typedef TestResult* (*OJI_TESTPROC)(void);


class OJITestLoader : nsISupports {
public:

    NS_DEFINE_STATIC_IID_ACCESSOR(OJITESTLOADER_IID)

    NS_DECL_ISUPPORTS

    OJITestLoader();
    virtual ~OJITestLoader();
    
    static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

private:
	char** loadTestList();
	TestResult* runTest(const char* testCase, const char* libName);
	int countChars(char* buf, char ch);	
	void registerRes(TestResult *res, char *testCase);
	TEST_TYPE getTestType(char *tc);
private:
	PRFileDesc *fdResFile;
};


#endif 

