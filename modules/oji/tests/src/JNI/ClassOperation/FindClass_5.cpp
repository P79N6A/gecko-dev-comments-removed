



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_5)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");

  if(clazz==NULL){
      return TestResult::FAIL("FindClass(name of existing abstarct class) return NULL - incorrect");
  }
  return TestResult::PASS("FindClass(name of existing abstarct class) return correct value");

}
