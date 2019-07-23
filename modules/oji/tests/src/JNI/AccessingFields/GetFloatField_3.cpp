



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFloatField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");
  jfloat value = env->GetFloatField(obj, (jfieldID)-100);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetFloatField with fieldID = (jfieldID)-100 return correct value - do not crash");

}
