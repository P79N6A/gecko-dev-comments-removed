



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticFloatField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_float", "F");
  env->SetStaticFloatField(clazz, (jfieldID)100, 1);
  printf("value = %d\n", (int)env->GetStaticFloatField(clazz, fieldID));

  return TestResult::PASS("SetStaticFloatField(fieldID == (jfieldID)100) do not crush");

}
