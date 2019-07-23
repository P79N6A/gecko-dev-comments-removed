



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticShortField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  env->SetStaticShortField(clazz, (jfieldID)100, 1);
  printf("value = %d\n", env->GetStaticShortField(clazz, fieldID));

  return TestResult::PASS("SetStaticShortField(fieldID == (jfieldID)100) do not crush");

}
