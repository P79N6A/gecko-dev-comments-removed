



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_IsInstanceOf_5)
{
  GET_JNI_FOR_TEST

  jclass clazz1 = env->FindClass("Test1");
  jclass clazz2 = env->FindClass("Test2");
  jclass clazz3 = env->FindClass("Test5");
  jobject obj1 = env->AllocObject(clazz1);
  jobject obj2 = env->AllocObject(clazz2);
  

  if(env->IsInstanceOf(NULL, NULL)){
      return TestResult::PASS("IsInstanceOf(NULL, NULL) return correct value");
  }else{
      return TestResult::FAIL("IsInstanceOf(NULL, NULL) return incorrect value");
  }


}
