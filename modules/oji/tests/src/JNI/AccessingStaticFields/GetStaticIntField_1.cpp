



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticIntField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "I");
  jint value = env->GetStaticIntField(NULL, fieldID);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticIntField(clazz == NULL) do not crush");

}
