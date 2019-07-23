



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");

  if(clazz==NULL){
      return TestResult::FAIL("FindClass(name of existing class) return NULL - incorrect");
  }
  return TestResult::PASS("FindClass(name of existing class) return correct value");

}
