



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_GetObjectClass_2)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);

  jclass clazz1 = env->GetObjectClass(NULL);

  if(clazz1 == NULL){
      return TestResult::PASS("GetObjectClass(NULL) return correct value");
  }else{
      return TestResult::FAIL("GetObjectClass(NULL) return incorrect value");
  }


}
