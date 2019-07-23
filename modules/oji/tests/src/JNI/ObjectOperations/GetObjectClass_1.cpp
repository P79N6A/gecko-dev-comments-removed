



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_GetObjectClass_1)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);

  jclass clazz1 = env->GetObjectClass(obj);

  if(env->IsAssignableFrom(clazz1, clazz)) {
      return TestResult::PASS("GetObjectClass(some object) return correct value");
  }else{
      return TestResult::FAIL("GetObjectClass(some object) return incorrect value");
  }


}
