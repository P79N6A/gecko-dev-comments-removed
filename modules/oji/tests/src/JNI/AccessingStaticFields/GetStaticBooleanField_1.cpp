



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticBooleanField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  jboolean value = env->GetStaticBooleanField(NULL, fieldID);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticBooleanField(clazz == NULL) do not crush");

}
