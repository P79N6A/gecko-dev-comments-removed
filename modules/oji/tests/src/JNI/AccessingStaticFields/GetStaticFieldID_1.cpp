



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  jfieldID fieldID = env->GetStaticFieldID(NULL, "static_name_int", "I" );
  printf("fieldID = %d\n", (int)fieldID);

  return TestResult::PASS("GetStaticFieldID with class == NULL return correct value");

}
