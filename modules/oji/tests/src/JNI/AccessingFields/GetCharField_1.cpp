



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetCharField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");
  jchar value = env->GetCharField(NULL, fieldID);
  printf("value = %c\n", (char)value);
  return TestResult::PASS("GetCharField with obj = NULL return correct value - do not crash");

}
