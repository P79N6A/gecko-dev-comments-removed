



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", NULL, "(Ljava/lang/String;)V");
  


  if(MethodID == NULL){
    return TestResult::PASS("GetStaticMethodID for name = NULL return 0, it's correct");
  }else{
    return TestResult::FAIL("GetStaticMethodID for name = NULL doesn't return 0, it's incorrect");
  }
}
