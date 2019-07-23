



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetByteField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_byte", "B");
  jbyte value = env->GetByteField(obj, (jfieldID)100);
  printf("value = %d\n", (byte)value);
  return TestResult::PASS("GetByteField with fieldID = (jfieldID)=100 return correct value - do not crash");

}
