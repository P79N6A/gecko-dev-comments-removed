



































#include "JNIEnvTests.h"
#include "GlobalAndLocalRefs.h"

JNI_OJIAPITest(JNIEnv_DeleteGlobalRef_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  jobject obj_ref = env->NewGlobalRef(obj);
  env->DeleteGlobalRef(NULL);

  return TestResult::PASS("DeleteGlobalRef(NULL) passed");

}
