



































#include <ThreadManagerTests.h>

TM_OJIAPITest(ThreadManager_ExitMonitor_1) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->ExitMonitor(NULL);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because adress is invalid (NULL).");
	return TestResult::FAIL("ExitMonitor", rc);
}

TM_OJIAPITest(ThreadManager_ExitMonitor_2) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->ExitMonitor(threadMgr);
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Thread can exit monitor even if it doesn't own it.");
	return TestResult::FAIL("ExitMonitor", rc);

}

TM_OJIAPITest(ThreadManager_ExitMonitor_3) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_SUCCEEDED(rc)) {
		rc = threadMgr->ExitMonitor(threadMgr);
		if (NS_SUCCEEDED(rc))
			return TestResult::PASS("Method should work OK.");
		return TestResult::FAIL("ExitMonitor", rc);
	}
	return TestResult::FAIL("ExitMonitor", "Can't enter monitor", rc);

}

TM_OJIAPITest(ThreadManager_ExitMonitor_4) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		DummyThread(nsIThreadManager *threadMgr, nsresult def_rc) {
			tm = threadMgr;
			rc = def_rc;
		} 
		NS_METHOD Run() { 
			
			tm->EnterMonitor(tm); 
			rc = NS_OK;
			
			while(1); 
			return NS_OK;
		}
	};
	PRUint32 id = 0;
	DummyThread *newThread = new DummyThread(threadMgr, NS_ERROR_FAILURE);

	
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_SUCCEEDED(rc)) {
		rc = threadMgr->CreateThread(&id, (nsIRunnable*)newThread);
		if (NS_SUCCEEDED(rc))  {
			
			threadMgr->Sleep(500);
			rc = threadMgr->ExitMonitor(threadMgr);
			if (NS_SUCCEEDED(rc)) {
				threadMgr->Sleep(500);
				rc = newThread->rc;
				if (NS_SUCCEEDED(rc))
					return TestResult::PASS("Another thread CAN enter monitor if the current thread unlock it.");
				return TestResult::FAIL("ExitMonitor", rc);
			}
			
			return TestResult::FAIL("ExitMonitor", "First thread can't exit monitor", rc);
		}
		
		return TestResult::FAIL("ExitMonitor", "Can't create new thread", rc);

	}
	
	return TestResult::FAIL("ExitMonitor", "First thread can't enter monitor", rc);
}



