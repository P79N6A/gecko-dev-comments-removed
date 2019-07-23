



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetShortField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");
  jshort value = env->GetShortField(obj, (jfieldID)100);
  printf("value = %d\n", (short)value);
  return TestResult::PASS("GetShortField with fieldID = (jfieldID)100 return correct value - do not crash");

}
