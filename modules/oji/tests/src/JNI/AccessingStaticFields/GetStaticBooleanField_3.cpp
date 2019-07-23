



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticBooleanField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  jboolean value = env->GetStaticBooleanField(clazz, (jfieldID)100);
  printf("value = %d\n", (int)value);

  return TestResult::PASS("GetStaticBooleanField(fieldID == (jfieldID)100) do not crush");

}
