



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_IsInstanceOf_2)
{
  GET_JNI_FOR_TEST

  jclass clazz1 = env->FindClass("Test1");
  jclass clazz2 = env->FindClass("Test2");
  jobject obj1 = env->AllocObject(clazz1);
  jobject obj2 = env->AllocObject(clazz2);
  

  if(env->IsInstanceOf(obj1, clazz2)){
      return TestResult::PASS("IsInstanceOf(some obj, its superclass) return correct value");
  }else{
      return TestResult::FAIL("IsInstanceOf(some obj, its superclass) return incorrect value");
  }


}
