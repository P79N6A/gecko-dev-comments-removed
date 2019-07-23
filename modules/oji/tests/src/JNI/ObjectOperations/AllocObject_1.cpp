



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_AllocObject_1)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jobject obj = env->AllocObject(NULL);
  printf("AllocObject passed");
  if(obj == NULL){
    return TestResult::PASS("AllocObject(NULL) return correct value - NULL");
  }else{
    return TestResult::FAIL("AllocObject(NULL) do not return correct value - NULL");
  }
}
