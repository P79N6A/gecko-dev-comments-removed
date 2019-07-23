



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticCharField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  env->SetStaticCharField(clazz, (jfieldID)-100, 'a');
  printf("value = %c\n", (char)env->GetStaticCharField(clazz, fieldID));

  return TestResult::PASS("SetStaticCharField(fieldID == (jfieldID)-100) do not crush");

}
