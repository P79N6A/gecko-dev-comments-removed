



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetShortField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");
  jshort value = env->GetShortField(NULL, fieldID);
  printf("value = %d\n", (short)value);
  return TestResult::PASS("GetShortField with obj = NULL return correct value - do not crash");

}
