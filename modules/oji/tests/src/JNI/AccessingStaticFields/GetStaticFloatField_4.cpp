



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFloatField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_float", "F");
  jint value = env->GetStaticFloatField(clazz, (jfieldID)100);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticFloatField(fieldID == (jfieldID)100) do not crush");

}
