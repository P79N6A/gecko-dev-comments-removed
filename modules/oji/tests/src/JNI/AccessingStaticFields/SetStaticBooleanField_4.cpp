



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticBooleanField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  env->SetStaticBooleanField(clazz, NULL, TRUE);
  printf("value = %d\n", (int)env->GetStaticBooleanField(clazz, fieldID));

  return TestResult::PASS("SetStaticBooleanField(fieldID == NULL) do not crush");

}
