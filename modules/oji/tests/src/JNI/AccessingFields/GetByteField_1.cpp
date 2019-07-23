



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetByteField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_byte", "B");
  jbyte value = env->GetByteField(NULL, fieldID);
  printf("value = %d\n", (byte)value);
  return TestResult::PASS("GetByteField with obj = NULL return correct value - do not crash");

}
