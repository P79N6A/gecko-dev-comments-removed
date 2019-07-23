



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetShortField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");
  jshort value = env->GetShortField(obj, NULL);
  printf("value = %d\n", (short)value);
  return TestResult::PASS("GetShortField with fieldID = NULL return correct value - do not crash");

}
