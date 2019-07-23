



































#include <ThreadManagerTests.h>



TM_OJIAPITest(ThreadManager_GetCurrentThread_2) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->GetCurrentThread(NULL);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no memory is allocated for the result pointer.");
	return TestResult::FAIL("GetCurrentThread", rc);

}

TM_OJIAPITest(ThreadManager_GetCurrentThread_1) {
	GET_TM_FOR_TEST
	PRUint32 *id;

	nsresult rc = threadMgr->GetCurrentThread((nsPluginThread**)&id);
	
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should work OK.");
	return TestResult::FAIL("GetCurrentThread", rc);

}
