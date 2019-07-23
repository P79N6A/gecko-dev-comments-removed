



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method1", "()V");
  env->CallVoidMethod(obj, MethodID, NULL);

  if(MethodID!=NULL){
    return TestResult::PASS("GetMethodID for public not inherited method (sig = ()V) return correct value");
  }else{
    return TestResult::FAIL("GetMethodID for public not inherited method (sig = ()V) return incorrect value");
  }
}
