



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_AllocObject_2)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  printf("AllocObject passed");
  if(obj!=NULL){
    return TestResult::PASS("AllocObject(correct class) return correct value");
  }else{
    return TestResult::FAIL("AllocObject(correct class) return incorrect value");
  }
}
