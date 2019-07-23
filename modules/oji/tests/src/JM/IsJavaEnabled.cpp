



































#include "JVMManagerTests.h"



JM_OJIAPITest(JVMManager_IsJavaEnabled_1) {
	GET_JM_FOR_TEST

	nsresult rc = jvmMgr->IsJavaEnabled(nsnull);
	if (NS_FAILED(rc))
		return TestResult::PASS("Method should fail because no space is allocated for the result pointer.");
	return TestResult::FAIL("GetProxyJNI", rc);

}

JM_OJIAPITest(JVMManager_IsJavaEnabled_2) {
	GET_JM_FOR_TEST
	PRBool b;

	nsresult rc = jvmMgr->IsJavaEnabled(&b);
	if (NS_SUCCEEDED(rc) && b == PR_TRUE) {
		return TestResult::PASS("Should PASS if security.enable_java property is set to true or ommited.");
	}
	return TestResult::FAIL("IsJavaEnabled", rc);
}


JM_OJIAPITest(JVMManager_IsJavaEnabled_3) {
	GET_JM_FOR_TEST
	PRBool b;

	
	nsresult rc = jvmMgr->IsJavaEnabled(&b);
	if (NS_SUCCEEDED(rc) && b == PR_FALSE) {
		return TestResult::PASS("Should PASS if security.enable_java property is set to false.");
	}
	return TestResult::FAIL("To make this test pass one should delete OJI plugin !");

}







 
