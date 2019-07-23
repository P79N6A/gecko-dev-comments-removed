



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticCharField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  jchar value = env->GetStaticCharField(clazz, (jfieldID)100);
  printf("value = %c\n", (char)value);

  return TestResult::PASS("GetStaticCharField(fieldID == (jfieldID)100) do not crush");

}
