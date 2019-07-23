



































#include <LiveConnectManagerTests.h>


LCM_OJIAPITest(LCM_WrapJavaObject_1) { 
	GET_LCM_FOR_TEST
	nsresult rc = lcMgr->WrapJavaObject(nsnull, nsnull, nsnull);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because all parametrs are invalid.");
	return TestResult::FAIL("WrapJavaObject", rc);

}

LCM_OJIAPITest(LCM_WrapJavaObject_2) { 
	GET_LCM_FOR_TEST
	JSObject *out;
	nsresult rc = lcMgr->WrapJavaObject(nsnull, nsnull, &out);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because first two parametrs are invalid.");
	return TestResult::FAIL("WrapJavaObject", rc);

}


