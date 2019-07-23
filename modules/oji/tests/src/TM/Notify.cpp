



































#include <ThreadManagerTests.h>


TM_OJIAPITest(ThreadManager_Notify_1) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->Notify(NULL);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because adress is invalid (NULL)");
	return TestResult::FAIL("Notify", rc);

}


TM_OJIAPITest(ThreadManager_Notify_2) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->Notify(threadMgr);
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should fail because it can call Notify only being monitor's owner.");
	return TestResult::FAIL("Notify", rc);

}


TM_OJIAPITest(ThreadManager_Notify_3) {
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
				
				if (NS_SUCCEEDED(lrc) && notified)
					this->rc = NS_OK;
				tm->ExitMonitor(tm);
			} else {
				fprintf(stderr, "ERROR: new thread %p can't create monitor for adress %p\n", this, tm);
			}
			while(1);
			return NS_OK;
		}
	};
	PRUint32 id = 0;
	DummyThread *newThread1 = new DummyThread(threadMgr, NS_ERROR_FAILURE);

	nsresult rc  = threadMgr->CreateThread(&id, (nsIRunnable*)newThread1);
	if (NS_SUCCEEDED(rc)) {
		
		threadMgr->Sleep((PRUint32)500);
		rc = threadMgr->EnterMonitor(threadMgr);
		if (NS_SUCCEEDED(rc)) {
			newThread1->notified = 1;
			threadMgr->Notify(threadMgr);
			if (NS_SUCCEEDED(rc)) {
				rc = threadMgr->ExitMonitor(threadMgr);
				if (NS_SUCCEEDED(rc)) {
					
					threadMgr->Sleep((PRUint32)500);
					if (NS_SUCCEEDED(newThread1->rc))
						return TestResult::PASS("Method should work OK.");
					return TestResult::FAIL("Notify", rc);		
				} else {
					return TestResult::FAIL("Notify", "Can't exit monitor", rc);
				}
			} else {
				
				return TestResult::FAIL("Notify", rc);
			}
		} else {
			return TestResult::FAIL("Notify", "Can't enter monitor", rc);
		}
	}
	
	return TestResult::FAIL("Notify", "Can't create new threads", rc);
}



TM_OJIAPITest(ThreadManager_Notify_4) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	
	if (NS_SUCCEEDED(rc)) {
		nsresult rc = threadMgr->Notify(threadMgr);
		if (NS_SUCCEEDED(rc))
			return TestResult::PASS("Method should work even if there are no other threads waiting on monitor.");
		return TestResult::FAIL("Notify", rc);
	}
	return TestResult::FAIL("Notify", "Can't enter monitor", rc);
}




