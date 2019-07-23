



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticByteField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  jbyte value = env->GetStaticByteField(clazz, (jfieldID)-100);
  printf("value = %d\n", (byte)value);

  return TestResult::PASS("GetStaticByteField(fieldID == (jfieldID)-100) do not crush");

}
