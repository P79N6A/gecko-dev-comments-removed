



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_FindClass_8)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("[Ljava/lang/String;");

  if(clazz==NULL){
      return TestResult::FAIL("FindClass([java.lang.String) failed");
  }
  return TestResult::PASS("FindClass([java.lang.String) return correct value");

}
