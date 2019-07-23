



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticByteField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  env->SetStaticByteField(clazz, (jfieldID)-100, 1);
  printf("value = %d\n", (int)env->GetStaticByteField(clazz, fieldID));

  return TestResult::PASS("SetStaticByteField(fieldID == (jfieldID)-100) do not crush");

}
