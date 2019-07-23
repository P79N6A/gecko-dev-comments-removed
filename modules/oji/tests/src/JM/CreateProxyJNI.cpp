



































#include "JVMManagerTests.h"

#include "CreateProxyJNI.h"



JM_OJIAPITest(JVMManager_CreateProxyJNI_1) {
	GET_JM_FOR_TEST
	JNIEnv *jniEnv;

	nsresult rc = jvmMgr->CreateProxyJNI(nsnull, &jniEnv);	
	if (NS_SUCCEEDED(rc) && jniEnv)
		return TestResult::PASS("First paramenter CAN be NULL.");
	return TestResult::FAIL("CreateProxyJNI", rc);
}


JM_OJIAPITest(JVMManager_CreateProxyJNI_2) {
	GET_JM_FOR_TEST
	

	nsresult rc = jvmMgr->CreateProxyJNI(nsnull, nsnull);	
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no space is allocated for the result pointer.");
	return TestResult::FAIL("CreateProxyJNI", rc);

}



JM_OJIAPITest(JVMManager_CreateProxyJNI_3) {
	GET_JM_FOR_TEST
	
	nsISecureEnv *secureEnv = new nsDummySecureEnv();

	nsresult rc = jvmMgr->CreateProxyJNI(secureEnv, nsnull);	
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no space is allocated for the result pointer.");
	return TestResult::FAIL("CreateProxyJNI", rc);
}



JM_OJIAPITest(JVMManager_CreateProxyJNI_4) {
	GET_JM_FOR_TEST
	JNIEnv *jniEnv;
	nsISecureEnv *secureEnv = new nsDummySecureEnv();

	nsresult rc = jvmMgr->CreateProxyJNI(secureEnv, &jniEnv);	
	if (NS_SUCCEEDED(rc) && jniEnv)
		return TestResult::PASS("Method should work OK.");
	return TestResult::FAIL("CreateProxyJNI", rc);
}



 
