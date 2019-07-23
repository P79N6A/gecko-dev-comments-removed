



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticIntField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "I");
  jint value = env->GetStaticIntField(clazz, (jfieldID)100);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticIntField(fieldID == (jfieldID)100) do not crush");

}
