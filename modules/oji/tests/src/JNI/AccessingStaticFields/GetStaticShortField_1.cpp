



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticShortField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  jshort value = env->GetStaticShortField(NULL, fieldID);
  printf("value = %d\n", (short)value);

  return TestResult::PASS("GetStaticShortField(clazz == NULL) do not crush");

}
