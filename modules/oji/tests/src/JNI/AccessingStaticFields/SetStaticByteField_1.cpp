



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticByteField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  env->SetStaticByteField(NULL, fieldID,1);
  printf("value = %d\n", (int)env->GetStaticByteField(clazz, fieldID));

  return TestResult::PASS("SetStaticByteField(clazz == NULL) do not crush");

}
