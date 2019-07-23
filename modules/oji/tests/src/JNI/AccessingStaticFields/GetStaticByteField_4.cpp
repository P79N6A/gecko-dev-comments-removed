



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticByteField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  jbyte value = env->GetStaticByteField(clazz, NULL);
  printf("value = %d\n", (byte)value);

  return TestResult::PASS("GetStaticByteField(fieldID == NULL) do not crush");

}
