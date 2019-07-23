



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method1_static", "()V");
  env->CallStaticVoidMethod(clazz, MethodID, NULL);

  if(MethodID != NULL){
    return TestResult::PASS("GetStaticMethodID for public not inherited method (sig = ()V) return correct value");
  }else{
    return TestResult::FAIL("GetStaticMethodID for public not inherited method (sig = ()V) return incorrect value");
  }
}
