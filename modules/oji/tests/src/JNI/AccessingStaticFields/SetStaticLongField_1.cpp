



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticLongField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  env->SetStaticLongField(NULL, fieldID, 1);
  printf("value = %ld\n", env->GetStaticLongField(clazz, fieldID));

  return TestResult::PASS("SetStaticLongField(clazz == NULL) do not crush");

}
