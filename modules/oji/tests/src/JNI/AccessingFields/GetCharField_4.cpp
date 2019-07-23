



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetCharField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");
  jchar value = env->GetCharField(obj, (jfieldID)100);
  printf("value = %c\n", (char)value);
  return TestResult::PASS("GetCharField with fieldID = (jfieldID) return correct value - do not crash");

}
