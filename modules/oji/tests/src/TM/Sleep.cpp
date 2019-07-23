



































#include <ThreadManagerTests.h>



TM_OJIAPITest(ThreadManager_Sleep_1) {
	GET_TM_FOR_TEST
	PRUint32 id = 0;
	class DummyThread : public BaseDummyThread {
	private:
		nsIThreadManager *tm;
	public:
		int awoken;
		DummyThread(nsIThreadManager *aTm) : tm(aTm), awoken(0) {}
		NS_METHOD Run() {
			tm->Sleep(0);
			awoken = 1;
			return NS_OK;
		}
	};

	DummyThread *newThread = new DummyThread(threadMgr);
	nsresult rc  = threadMgr->CreateThread(&id, (nsIRunnable*)newThread);
	if (NS_SUCCEEDED(rc)) {
		rc = threadMgr->Sleep(500);        
		if (NS_SUCCEEDED(rc) && newThread->awoken) 
			return TestResult::PASS("Method should work OK.");
		return TestResult::FAIL("Sleep", rc);
	} 
	return TestResult::FAIL("Sleep", "Can't create new thread", rc);

}



TM_OJIAPITest(ThreadManager_Sleep_2) {
	GET_TM_FOR_TEST
	PRUint32 id = 0;
	class DummyThread : public BaseDummyThread {
	private:
		nsIThreadManager *tm;
	public:
		int isSleeping;
		DummyThread(nsIThreadManager *aTm) : tm(aTm), isSleeping(1) {} 
		NS_METHOD Run() {
			tm->Sleep(UINT_MAX);
			isSleeping = 0;
			return NS_OK;
		}
	};

	DummyThread *newThread = new DummyThread(threadMgr);
	nsresult rc  = threadMgr->CreateThread(&id, (nsIRunnable*)newThread);
	if (NS_SUCCEEDED(rc)) {
		rc = threadMgr->Sleep(500);        
		if (NS_SUCCEEDED(rc) && newThread->isSleeping)
			return TestResult::PASS("Method should work OK.");
		return TestResult::FAIL("Sleep", rc);
	} 
	return TestResult::FAIL("Sleep", "Can't create new thread", rc);

}
