



































#include <ThreadManagerTests.h>

TM_OJIAPITest(ThreadManager_NotifyAll_1) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->NotifyAll(NULL);
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should fail because adress is invalid (NULL)");
	return TestResult::FAIL("NotifyAll", rc);
}

TM_OJIAPITest(ThreadManager_NotifyAll_2) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->NotifyAll(threadMgr);
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should fail because it can call Notify only being monitor's owner.");
	return TestResult::FAIL("NotifyAll", rc);

}


TM_OJIAPITest(ThreadManager_NotifyAll_3) {
	GET_TM_FOR_TEST
	class DummyThread : public BaseDummyThread {
	public:
		int notified;
		DummyThread *otherThread;
		DummyThread(nsIThreadManager *threadMgr, nsresult def_rc) : notified(0) {
			tm = threadMgr;
			rc = def_rc; 
		}
		NS_METHOD Run() { 
			nsresult lrc = tm->EnterMonitor(tm);
			printf("Thread %p waiting for monitor %p\n", this, tm);
			lrc = tm->Wait(tm);
			if (NS_SUCCEEDED(lrc))  {
				printf("Thread %p: after wait -> notified: %d\n", this, notified);
				
				if (notified == 1) {  
					this->rc = NS_OK;
				}
				tm->ExitMonitor(tm);
				
			} else {
				printf("ERROR: thread %p: Wait method returned error (%d)\n", this, lrc);
			}
			while(1);
			return NS_OK;
		}
	};

	PRUint32 id = 0;
	DummyThread *newThread1 = new DummyThread(threadMgr, NS_ERROR_FAILURE);
	DummyThread *newThread2 = new DummyThread(threadMgr, NS_ERROR_FAILURE);
	nsresult rc1  = threadMgr->CreateThread(&id, (nsIRunnable*)newThread1);
	nsresult rc2  = threadMgr->CreateThread(&id, (nsIRunnable*)newThread2);

	newThread1->otherThread = newThread2;
	newThread2->otherThread = newThread1;
	
	threadMgr->Sleep((PRUint32)500);
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	if (NS_SUCCEEDED(rc)) {
		if (NS_SUCCEEDED(rc)) {
			newThread1->notified = 1;
			newThread2->notified = 1;		
			rc = threadMgr->NotifyAll(threadMgr);
			threadMgr->ExitMonitor(threadMgr);
			if (NS_SUCCEEDED(rc)) {
				
				threadMgr->Sleep((PRUint32)500);
				if (NS_SUCCEEDED(newThread1->rc) && NS_SUCCEEDED(newThread2->rc))
					return TestResult::PASS("Method should work OK.");
				return TestResult::FAIL("Not all threads were notified.");
			} else {
				
				return TestResult::FAIL("NotifyAll", rc);
			}
		} else {
			return TestResult::FAIL("NotifyAll", "Main thread can't exit monitor", rc);
		}
	} else {
		
		return TestResult::FAIL("NotifyAll", "Can't create new threds", rc);
	}
	
	return TestResult::FAIL("NotifyAll", "Can't enter monitor", rc);
	
}

TM_OJIAPITest(ThreadManager_NotifyAll_4) {
	GET_TM_FOR_TEST
	nsresult rc = threadMgr->EnterMonitor(threadMgr);
	
	if (NS_SUCCEEDED(rc)) {
		nsresult rc = threadMgr->NotifyAll(threadMgr);
		if (NS_SUCCEEDED(rc))
			return TestResult::PASS("Method should work even if there are no other threads waiting on monitor.");
		return TestResult::FAIL("NotifyAll", rc);
	}
	return TestResult::FAIL("NotifyAll", "Can't enter monitor", rc);

}



