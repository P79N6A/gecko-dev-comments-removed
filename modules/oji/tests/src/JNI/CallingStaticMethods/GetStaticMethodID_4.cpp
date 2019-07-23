



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Print_string_static", "");
  

  if(MethodID == NULL){
     return TestResult::PASS("GetStaticMethodID with incorrect sig return 0, it's correct");
  }else{
     return TestResult::FAIL("GetStaticMethodID with incorrect sig doesn't return 0, it's incorrect");
  }

}
