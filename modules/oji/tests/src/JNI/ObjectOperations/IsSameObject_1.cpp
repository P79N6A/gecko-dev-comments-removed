



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_IsSameObject_1)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->FindClass("Test1");
  jobject obj1 = env->AllocObject(clazz1);
  jobject obj2 = env->AllocObject(clazz1);

  if(env->IsSameObject(obj1, obj1) == JNI_TRUE){
      return TestResult::PASS("IsSameObject(obj, obj) return correct value");
  }else{
      return TestResult::FAIL("IsSameObject(obj, obj) return incorrect value");
  }


}
