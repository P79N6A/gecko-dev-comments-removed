



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticDoubleField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  env->SetStaticDoubleField(clazz, (jfieldID)100, 1);
  printf("value = %Lf\n", env->GetStaticDoubleField(clazz, fieldID));

  return TestResult::PASS("SetStaticDoubleField(fieldID == (jfieldID)100) do not crush");

}
