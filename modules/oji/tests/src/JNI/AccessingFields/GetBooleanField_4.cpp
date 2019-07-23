



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetBooleanField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");
  jboolean value = env->GetBooleanField(obj, NULL);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetBooleanField with fieldID = NULL return correct value - do not crash");

}
