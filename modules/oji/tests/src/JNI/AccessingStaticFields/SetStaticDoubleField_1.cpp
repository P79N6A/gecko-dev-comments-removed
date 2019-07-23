



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticDoubleField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  env->SetStaticDoubleField(NULL, fieldID, 1);
  printf("value = %Lf\n", env->GetStaticDoubleField(clazz, fieldID));

  return TestResult::PASS("SetStaticDoubleField(clazz == NULL) do not crush");

}
