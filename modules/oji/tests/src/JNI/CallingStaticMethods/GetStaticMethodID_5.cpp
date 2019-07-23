



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "name_not_exist", "(Ljava/lang/String;)V");
  

  if(MethodID == NULL){
    return TestResult::PASS("GetStaticMethodID for not existing name return 0, it's correct");
  }else{
    return TestResult::FAIL("GetStaticMethodID for not existing name doesn't return 0, it's incorrect");
  }
}
