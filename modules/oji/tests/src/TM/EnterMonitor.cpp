



































#include <ThreadManagerTests.h>


TM_OJIAPITest(ThreadManager_EnterMonitor_1) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->EnterMonitor(NULL);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method shoulf fail because invalid (NULL) adress is specified.");
	return TestResult::FAIL("EnterMonitor", rc);

}

TM_OJIAPITest(ThreadManager_EnterMonitor_2) {
	GET_TM_FOR_TEST
	
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Current thread can enter monitor even if no other thread we created.");
	return TestResult::FAIL("EnterMonitor", rc);

}

TM_OJIAPITest(ThreadManager_EnterMonitor_3) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	printf("Entering monitor with adress %p\n", threadMgr);
	if (NS_SUCCEEDED(rc)) {
		printf("Entering monitor again with adress %p\n", threadMgr);
		nsresult rc = threadMgr->EnterMonitor(threadMgr);
		if (NS_SUCCEEDED(rc))
			return TestResult::PASS("One thread can enter monitor twice.");
		return TestResult::FAIL("EnterMonitor", rc);
	}
	return TestResult::FAIL("EnterMonitor", "Can't enter monitor at all", rc);

}


TM_OJIAPITest(ThreadManager_EnterMonitor_4) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		DummyThread(nsIThreadManager *threadMgr, nsresult def_rc) {
			tm = threadMgr;
			rc = def_rc; 
		}
		NS_METHOD Run() { 
			
			tm->EnterMonitor(tm); 
			rc = NS_ERROR_FAILURE;
			
			while(1); 
			return NS_OK;
		}
	};
	PRUint32 id = 0;
	DummyThread *newThread = new DummyThread(threadMgr, NS_OK);

	printf("Our thread enters monitor with adress %p (%d) ...\n", threadMgr, id);
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_SUCCEEDED(rc)) {
		rc = threadMgr->CreateThread(&id, (nsIRunnable*)newThread);
		if (NS_SUCCEEDED(rc))  {
			
			threadMgr->Sleep(1000);
			rc = newThread->rc;
			if (NS_SUCCEEDED(rc))
				return TestResult::PASS("Another thread can't enter monitor locked by current thread.");
			return TestResult::FAIL("EnterMonitor", rc);

		}
		return TestResult::FAIL("EnterMonitor", "Can't create new thread", rc);

	}
	printf("First thread can't enter monitor ...\n");
	return TestResult::FAIL("EnterMonitor", "First thread can't enter monitor", rc);

}

