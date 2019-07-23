



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticIntField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "I");
  env->SetStaticIntField(clazz, NULL, 1);
  printf("value = %d\n", (int)env->GetStaticIntField(clazz, fieldID));

  return TestResult::PASS("SetStaticIntField(fieldID == NULL) do not crush");

}
