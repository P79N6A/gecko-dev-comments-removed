



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_IsAssignableFrom_4)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->FindClass("Test2");

  if(env->IsAssignableFrom(clazz, clazz) ){
      return TestResult::PASS("IsAssignableFrom(class, same class) return correct value");
  }else{
      return TestResult::FAIL("IsAssignableFrom(class, same class) return incorrect value");
  }



}
