



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticShortField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  jshort value = env->GetStaticShortField(clazz, (jfieldID)-100);
  printf("value = %d\n", (short)value);

  return TestResult::PASS("GetStaticShortField(fieldID == (jfieldID)-100) do not crush");

}
