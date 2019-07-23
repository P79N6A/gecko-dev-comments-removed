



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticCharField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  jchar value = env->GetStaticCharField(NULL, fieldID);
  printf("value = %c\n", (char)value);

  return TestResult::PASS("GetStaticCharField(clazz == NULL) do not crush");

}
