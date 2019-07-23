



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", NULL);

  if(MethodID == NULL){
     return TestResult::PASS("GetMethodID with sig==NULL return 0, it's correct");
  }else{
     return TestResult::FAIL("GetMethodID with sig==NULL doesn't return 0, it's incorrect");
  }

}
