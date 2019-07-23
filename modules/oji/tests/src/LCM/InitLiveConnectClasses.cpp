



































#include <LiveConnectManagerTests.h>


LCM_OJIAPITest(LCM_InitLiveConnectClasses_1) { 
	GET_LCM_FOR_TEST
	nsresult rc = lcMgr->InitLiveConnectClasses(nsnull, nsnull);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because parameters are invalid (NULLs).");
	return TestResult::FAIL("InitLiveConnectClasses", rc);


}
