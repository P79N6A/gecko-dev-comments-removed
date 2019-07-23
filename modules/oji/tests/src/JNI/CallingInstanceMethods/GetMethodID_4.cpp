



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "");
  

  if(MethodID == NULL){
     return TestResult::PASS("GetMethodID with incorrect sig return 0, it's correct");
  }else{
     return TestResult::FAIL("GetMethodID with incorrect sig doesn't return 0, it's incorrect");
  }

}
