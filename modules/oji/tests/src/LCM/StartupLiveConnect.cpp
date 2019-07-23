



































#include <LiveConnectManagerTests.h>


LCM_OJIAPITest(LCM_StartupLiveConnect_1) { 
	GET_LCM_FOR_TEST
	PRBool *out = nsnull;
	nsresult rc = lcMgr->StartupLiveConnect(nsnull, *out);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no memory is allocated for the result value.");
	return TestResult::FAIL("StartupLiveConnect", rc);
}

LCM_OJIAPITest(LCM_StartupLiveConnect_2) { 
	GET_LCM_FOR_TEST
	PRBool out;
	nsresult rc = lcMgr->StartupLiveConnect(nsnull, out);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because first parameter is invalid.");
	return TestResult::FAIL("StartupLiveConnect", rc);

}
