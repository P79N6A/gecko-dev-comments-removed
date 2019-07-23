



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_IsAssignableFrom_2)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->GetSuperclass(clazz);

  if(env->IsAssignableFrom(NULL, NULL) ){
      return TestResult::FAIL("IsAssignableFrom(NULL, NULL) return incorrect value");
  }else{
      return TestResult::PASS("IsAssignableFrom(NULL, NULL) return correct value");
  }



}
