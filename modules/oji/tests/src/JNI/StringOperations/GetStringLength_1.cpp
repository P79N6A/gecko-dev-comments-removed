



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringLength_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jsize len = env->GetStringLength(NULL);
  if((int)len==0){
    return TestResult::PASS("GetStringLength(NULL) returns correct value");
  }else{
    return TestResult::FAIL("GetStringLength(NULL) returns incorrect value");
  }

}
