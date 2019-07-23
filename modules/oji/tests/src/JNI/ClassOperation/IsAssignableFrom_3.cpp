



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_IsAssignableFrom_3)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->GetSuperclass(clazz);

  if(env->IsAssignableFrom(clazz1, clazz) ){
      return TestResult::FAIL("IsAssignableFrom(class, is not superclass of class) return incorrect value");
  }else{
      return TestResult::PASS("IsAssignableFrom(class, is not superclass of class) return correct value");
  }



}
