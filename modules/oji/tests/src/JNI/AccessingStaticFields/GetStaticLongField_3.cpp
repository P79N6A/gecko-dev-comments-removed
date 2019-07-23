



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticLongField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  jlong value = env->GetStaticLongField(clazz, (jfieldID)-100);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticLongField(fieldID == (jfieldID)-100) do not crush");

}
