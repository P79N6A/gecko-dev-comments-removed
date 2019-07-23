



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_IsAssignableFrom_5)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test5");
  jclass clazz1 = env->FindClass("Test3");

  if(env->IsAssignableFrom(clazz, clazz1) ){
      return TestResult::PASS("IsAssignableFrom(implements of interface, interface) return correct value");
  }else{
      return TestResult::FAIL("IsAssignableFrom(implements of interface, interface) return incorrect value");
  }



}
