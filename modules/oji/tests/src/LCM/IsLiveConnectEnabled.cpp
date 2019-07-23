



































#include <LiveConnectManagerTests.h>


LCM_OJIAPITest(LCM_IsLiveConnectEnabled_1) { 
	GET_LCM_FOR_TEST
	PRBool *out = nsnull;
	nsresult rc = lcMgr->IsLiveConnectEnabled(*out);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no memory is allocated for the result value.");
	return TestResult::FAIL("IsLiveConnectEnabled", rc);

}

LCM_OJIAPITest(LCM_IsLiveConnectEnabled_2) { 
	GET_LCM_FOR_TEST
	PRBool out;
	nsresult rc = lcMgr->IsLiveConnectEnabled(out);
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should work OK.");
	return TestResult::FAIL("IsLiveConnectEnabled", rc);

}

