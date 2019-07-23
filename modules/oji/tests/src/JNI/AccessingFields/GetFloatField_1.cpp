



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFloatField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");
  jfloat value = env->GetFloatField(NULL, fieldID);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetFloatField with obj = NULL return correct value - do not crash");

}
