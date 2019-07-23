



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", NULL, "(Ljava/lang/String;)V");
  


  if(MethodID == NULL){
    return TestResult::PASS("GetMethodID for name = NULL return 0, it's correct");
  }else{
    return TestResult::FAIL("GetMethodID for name = NULL doesn't return 0, it's incorrect");
  }
}
