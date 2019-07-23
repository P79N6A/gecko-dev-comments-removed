



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFloatField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");
  jfloat value = env->GetFloatField(obj, NULL);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetFloatField with fieldID = NULL return correct value - do not crash");

}
