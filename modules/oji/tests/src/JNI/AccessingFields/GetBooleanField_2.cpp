



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetBooleanField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");
  jboolean value = env->GetBooleanField(obj, (jfieldID)-100);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetBooleanField with fieldID = (jfieldID)-100 return correct value - do not crash");

}
