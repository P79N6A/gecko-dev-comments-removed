



































#include "JNIEnvTests.h"
#include "GlobalAndLocalRefs.h"

JNI_OJIAPITest(JNIEnv_DeleteLocalRef_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  jobject obj_ref = env->NewGlobalRef(obj);
  env->DeleteLocalRef(NULL);

  return TestResult::PASS("DeleteLocalRef(NULL) passed");

}
