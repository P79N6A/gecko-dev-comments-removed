



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_GetSuperclass_2)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->GetSuperclass(clazz);
  jclass clazz2 = env->FindClass("Test2");
  printf("GetSuperPassed");

  if(env->IsAssignableFrom(clazz1, clazz2)){
    return TestResult::PASS("GetSuperclass(existing class) return correct value");
  }else{
    return TestResult::FAIL("GetSuperclass(existing class) return incorrect value");
  }
}
