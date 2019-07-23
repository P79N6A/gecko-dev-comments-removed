



































#include <LiveConnectManagerTests.h>


LCM_OJIAPITest(LCM_ShutdownLiveConnect_1) { 
	GET_LCM_FOR_TEST
	PRBool *out = nsnull;
	nsresult rc = lcMgr->ShutdownLiveConnect(nsnull, *out);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no memory is allocated for the result value (PRBool *out = nsnull;)");
	return TestResult::FAIL("ShutdownLiveConnect", rc);

}

LCM_OJIAPITest( LCM_ShutdownLiveConnect_2) { 
	GET_LCM_FOR_TEST
	PRBool out;
	nsresult rc = lcMgr->ShutdownLiveConnect(nsnull, out);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because first parameter is incorrect (NULL).");
	return TestResult::FAIL("ShutdownLiveConnect", rc);

}

