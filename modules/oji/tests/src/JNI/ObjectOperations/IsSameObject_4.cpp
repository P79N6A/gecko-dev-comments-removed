



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_IsSameObject_4)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->FindClass("Test1");
  jobject obj1 = env->AllocObject(clazz1);
  jobject obj2 = env->AllocObject(clazz1);
  jobject obj = env->AllocObject(clazz);

  if(env->IsSameObject(NULL, NULL) == JNI_FALSE){
      return TestResult::FAIL("IsSameObject(NULL, NULL) return incorrect value");
  }else{
      return TestResult::PASS("IsSameObject(NULL, NULL) return correct value");
  }


}
