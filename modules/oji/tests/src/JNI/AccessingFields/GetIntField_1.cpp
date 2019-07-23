



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetIntField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_int", "I");
  jint value = env->GetIntField(NULL, fieldID);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetIntField with obj = NULL return correct value - do not crash");

}
