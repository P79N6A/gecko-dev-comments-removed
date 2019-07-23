



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_4)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Testx");

  if(clazz!=NULL){
      return TestResult::FAIL("FindClass(name of non existing class) return not NULL - incorrect");
  }
  return TestResult::PASS("FindClass(name of non existing class) return NULL - correct");

}
