



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetCharField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");

  env->SetCharField(obj, (jfieldID)100, 'a');
  printf("value = %c\n", (char)env->GetCharField(obj, fieldID));

  return TestResult::PASS("SetCharField(fieldID = (jfieldID)100) do not crush - correct");

}
