



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_3)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("");

  if(clazz!=NULL){
      return TestResult::FAIL("FindClass(empty string) not return NULL - incorrect");
  }
  return TestResult::PASS("FindClass(empty string) return NULL - correct");

}
