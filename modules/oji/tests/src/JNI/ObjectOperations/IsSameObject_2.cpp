



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_IsSameObject_2)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->FindClass("Test1");
  jobject obj1 = env->AllocObject(clazz1);
  jobject obj2 = env->AllocObject(clazz1);
  jobject obj = env->AllocObject(clazz);

  if(env->IsSameObject(obj1, obj2) == JNI_TRUE){
      return TestResult::FAIL("IsSameObject(obj, another obj) return incorrect value");
  }else{
      return TestResult::PASS("IsSameObject(obj, another obj) return correct value");
  }


}
