



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticBooleanField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  env->SetStaticBooleanField(NULL, fieldID, TRUE);
  printf("value = %d\n", (int)env->GetStaticBooleanField(clazz, fieldID));

  return TestResult::PASS("SetStaticBooleanField(clazz == NULL) do not crush");

}
