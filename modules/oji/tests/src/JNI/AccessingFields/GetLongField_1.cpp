



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetLongField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");
  jlong value = env->GetLongField(NULL, fieldID);
  printf("value = %d\n", (long)value);
  return TestResult::PASS("GetLongField with obj = NULL return correct value - do not crash");

}
