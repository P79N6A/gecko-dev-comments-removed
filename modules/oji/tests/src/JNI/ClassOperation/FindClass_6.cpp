



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_6)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test3");

  if(clazz==NULL){
      return TestResult::FAIL("FindClass(name of existing interface) return NULL - incorrect");
  }
  return TestResult::PASS("FindClass(name of existing abstarct class) return correct value");

}
