



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Print_string_static", NULL);

  if(MethodID == NULL){
     return TestResult::PASS("GetStaticMethodID with sig==NULL return 0, it's correct");
  }else{
     return TestResult::FAIL("GetStaticMethodID with sig==NULL doesn't return 0, it's incorrect");
  }

}
