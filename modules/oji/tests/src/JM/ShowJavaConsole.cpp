



































#include "JVMManagerTests.h"




JM_OJIAPITest(JVMManager_ShowJavaConsole_1) {
	GET_JM_FOR_TEST
	JNIEnv *jniEnv;

	jvmMgr->CreateProxyJNI(nsnull, &jniEnv);	
	nsresult rc = jvmMgr->ShowJavaConsole();
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("CreateProxyJNI is called before ShowJavaConsole.");
	return TestResult::FAIL("ShowJavaConsole", rc);

}

JM_OJIAPITest(JVMManager_ShowJavaConsole_2) {
	GET_JM_FOR_TEST
	JNIEnv *jniEnv;

	nsresult rc = jvmMgr->ShowJavaConsole();
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should PASS nevertheless CreateProxyJNI wasn't call before.");
	return TestResult::FAIL("ShowJavaConsole", rc);

}




 
