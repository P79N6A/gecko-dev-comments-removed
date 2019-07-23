



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetObjectField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_string", "Ljava/lang/String;");
  jstring value = (jstring)env->GetObjectField(NULL, fieldID);
  printf("value = %d\n", (int)value);
  return TestResult::PASS("GetObjectField with obj = NULL return correct value - do not crash");

}
