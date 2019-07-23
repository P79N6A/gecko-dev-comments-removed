



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetLongField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");
  jlong value = env->GetLongField(obj, NULL);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetLongField with fieldID = NULL return correct value - do not crash");

}
