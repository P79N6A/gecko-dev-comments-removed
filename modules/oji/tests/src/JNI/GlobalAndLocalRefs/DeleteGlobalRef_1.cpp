



































#include "JNIEnvTests.h"
#include "GlobalAndLocalRefs.h"

JNI_OJIAPITest(JNIEnv_DeleteGlobalRef_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  jobject obj_ref = env->NewGlobalRef(obj);
  env->DeleteGlobalRef(obj_ref);

  
  return TestResult::PASS("DeleteGlobalRef work properly with correct parameter");

}
