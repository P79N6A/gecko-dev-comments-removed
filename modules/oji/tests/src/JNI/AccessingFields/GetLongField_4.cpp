



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetLongField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");
  jlong value = env->GetLongField(obj, (jfieldID)100);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetLongField with fieldID = (jfieldID)100 return correct value - do not crash");

}
