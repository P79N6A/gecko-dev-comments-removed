



































#include <ThreadManagerTests.h>


TM_OJIAPITest(ThreadManager_CreateThread_1) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->CreateThread(NULL, NULL);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no nsIRunnable object is specified.");
	return TestResult::FAIL("CreateThread", rc);

}

TM_OJIAPITest(ThreadManager_CreateThread_2) {
	GET_TM_FOR_TEST
	PRUint32 id;

	nsresult rc = threadMgr->CreateThread(&id, NULL);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no nsIRunnable object is specified.");
	return TestResult::FAIL("CreateThread", rc);

}

TM_OJIAPITest(ThreadManager_CreateThread_3) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		DummyThread() {}
		NS_METHOD Run() { while(1); return NS_OK; }
	};

	DummyThread *dt = new DummyThread();
	nsresult rc = threadMgr->CreateThread(NULL, (nsIRunnable*)dt);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no space is allocated for thread id.");
	return TestResult::FAIL("CreateThread", rc);
	
}

TM_OJIAPITest(ThreadManager_CreateThread_4) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		DummyThread() {}
		NS_METHOD Run() { while(1); return NS_OK; }
	};
	PRUint32 id = 0;
	DummyThread *dt = new DummyThread();
	nsresult rc = threadMgr->CreateThread(&id, (nsIRunnable*)dt);	
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should work OK.");
	return TestResult::FAIL("");

}
