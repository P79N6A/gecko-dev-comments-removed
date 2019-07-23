



































#include <ThreadManagerTests.h>



TM_OJIAPITest(ThreadManager_Wait_1) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->Wait(NULL, (PRUint32)100);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because invalid adress (NULL) is specified.");
	return TestResult::FAIL("Wait", rc);

}

TM_OJIAPITest(ThreadManager_Wait_2) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		DummyThread(nsIThreadManager *aTM) { rc = NS_OK; tm = aTM; }
		NS_METHOD Run() {
			tm->EnterMonitor(tm);
			tm->Wait(tm, (PRUint32)UINT_MAX);
			rc = NS_ERROR_FAILURE; 
			tm->ExitMonitor(tm);
			return NS_OK; 
		}
	};
	PRUint32 id = 0;
	nsresult rc;

	DummyThread *dt = new DummyThread(threadMgr);
	rc = threadMgr->CreateThread(&id, (nsIRunnable*)dt);
	threadMgr->Sleep(500); 
	rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_FAILED(rc))
		return TestResult::FAIL("Wait","Can't eneter monitor", rc);
	threadMgr->Sleep(500);
	if (NS_SUCCEEDED(dt->rc))
		return TestResult::PASS("Method should work OK.");
	rc = threadMgr->ExitMonitor(threadMgr);
	if (NS_FAILED(rc))
		return TestResult::FAIL("Wait","Can't exit monitor", rc);
	return TestResult::FAIL("Wait", dt->rc);
}

TM_OJIAPITest(ThreadManager_Wait_3) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		DummyThread(nsIThreadManager *aTM) { rc = NS_OK; tm = aTM; }
		NS_METHOD Run() {
			tm->EnterMonitor(tm);
			tm->Wait(tm, (PRUint32)0);
			rc = NS_ERROR_FAILURE; 
			tm->ExitMonitor(tm);
			return NS_OK; 
		}
	};
	PRUint32 id = 0;
	nsresult rc;

	DummyThread *dt = new DummyThread(threadMgr);
	rc = threadMgr->CreateThread(&id, (nsIRunnable*)dt);
	threadMgr->Sleep(500); 
	rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_FAILED(rc))
		return TestResult::FAIL("Wait","Can't eneter monitor", rc);
	threadMgr->Sleep(500);
	if (NS_SUCCEEDED(dt->rc))
		return TestResult::PASS("Method should work OK.");
	rc = threadMgr->ExitMonitor(threadMgr);
	if (NS_FAILED(rc))
		return TestResult::FAIL("Wait","Can't exit monitor", rc);
	return TestResult::FAIL("Wait", dt->rc);
}

TM_OJIAPITest(ThreadManager_Wait_4) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->Wait(threadMgr, (PRUint32)100);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because current thread doesn't own monitor.");
	return TestResult::FAIL("Wait", rc);

}



TM_OJIAPITest(ThreadManager_Wait_5) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		int notified;
		DummyThread(nsIThreadManager *threadMgr, nsresult def_rc) : notified(0) {
			tm = threadMgr;
			rc = def_rc; 
		}
		NS_METHOD Run() { 
			nsresult lrc = tm->EnterMonitor(tm);
			if (NS_SUCCEEDED(lrc))  {
				lrc = tm->Wait(tm);
				
				this->rc = NS_ERROR_FAILURE;
			}
			while(1);
			return NS_OK;
		}
	};
	PRUint32 id = 0;
	DummyThread *newThread = new DummyThread(threadMgr, NS_OK);

	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_SUCCEEDED(rc)) {
		rc  = threadMgr->CreateThread(&id, (nsIRunnable*)newThread);
		if (NS_SUCCEEDED(rc)) {
			threadMgr->Sleep((PRUint32)500);
			if (NS_SUCCEEDED(newThread->rc))
				return TestResult::PASS("Method should work OK.");
			return TestResult::FAIL("Wait", rc);
		} else {
			return TestResult::FAIL("Wait", "Can't create new thread", rc);
		}
	}
	return TestResult::FAIL("Wait", "Can't enter moniotor", rc);
}
