



































#include "JVMManagerTests.h"



JM_OJIAPITest(JVMManager_GetProxyJNI_1) {
	GET_JM_FOR_TEST
	nsresult rc = jvmMgr->GetProxyJNI(nsnull);	
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no space is allocated for the result pointer.");
	return TestResult::FAIL("GetProxyJNI", rc);
}

JM_OJIAPITest(JVMManager_GetProxyJNI_2) {
	GET_JM_FOR_TEST
	JNIEnv *jniEnv;

	nsresult rc = jvmMgr->GetProxyJNI(&jniEnv);	
	if (NS_SUCCEEDED(rc))
		return TestResult::PASS("Method should work OK though we didn't call CreateProxyJNI before.");
	return TestResult::FAIL("GetProxyJNI", rc);

}
                               
JM_OJIAPITest(JVMManager_GetProxyJNI_3) {
	GET_JM_FOR_TEST
	JNIEnv *jniEnv;
	nsresult rc = jvmMgr->CreateProxyJNI(nsnull, &jniEnv);

	if (NS_SUCCEEDED(rc)) {
		rc = jvmMgr->GetProxyJNI(&jniEnv);	
		if (NS_SUCCEEDED(rc))
			return TestResult::PASS("Before calling GetProxyJNI CreateProxyJNI method is called.");
		return TestResult::FAIL("GetProxyJNI", rc);		
	}
	return TestResult::FAIL("Can't create ProxyJNI.");		
}



 
