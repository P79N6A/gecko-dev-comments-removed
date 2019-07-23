



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass(NULL);

  if(clazz!=NULL){
      return TestResult::FAIL("FindClass(NULL) doesn't return NULL - incorrect");
  }
  return TestResult::PASS("FindClass(NULL) return NULL - correct");

}
