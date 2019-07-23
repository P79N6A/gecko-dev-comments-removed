



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticLongField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  jlong value = env->GetStaticLongField(NULL, fieldID);
  printf("value = %d\n", (long)value);

  return TestResult::PASS("GetStaticLongField(clazz == NULL) do not crush");

}
