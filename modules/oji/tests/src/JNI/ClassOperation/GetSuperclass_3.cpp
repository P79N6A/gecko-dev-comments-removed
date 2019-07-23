



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_GetSuperclass_3)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test3");

  jclass clazz1 = env->GetSuperclass(clazz);
  if(clazz1!=NULL){
      return TestResult::FAIL("GetSuperclass(interface) do not return NULL - incorrect");
  }else{
      return TestResult::PASS("GetSuperclass(interface) return NULL - correct");
  }
}
