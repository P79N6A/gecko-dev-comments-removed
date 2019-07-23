



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_IsInstanceOf_3)
{
  GET_JNI_FOR_TEST

  jclass clazz1 = env->FindClass("Test1");
  jclass clazz2 = env->FindClass("Test2");
  jclass clazz3 = env->FindClass("Test5");
  jobject obj1 = env->AllocObject(clazz1);
  jobject obj2 = env->AllocObject(clazz2);
  

  if(env->IsInstanceOf(obj1, clazz3)){
      return TestResult::FAIL("IsInstanceOf(some obj, isn't superclass or class) return incorrect value");
  }else{
      return TestResult::PASS("IsInstanceOf(some obj, isn't superclass or class) return correct value");
  }


}
