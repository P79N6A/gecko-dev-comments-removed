



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFloatField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_float", "F");
  jint value = env->GetStaticFloatField(NULL, fieldID);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticFloatField(clazz == NULL) do not crush");

}
