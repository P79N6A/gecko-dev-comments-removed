



































#include "JNIEnvTests.h"
#include "GlobalAndLocalRefs.h"

JNI_OJIAPITest(JNIEnv_DeleteLocalRef_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  env->DeleteLocalRef(obj);

  return TestResult::PASS("DeleteLocalRef working properly with correct parameter");
}
