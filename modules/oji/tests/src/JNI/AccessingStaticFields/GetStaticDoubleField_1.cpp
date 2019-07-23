



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticDoubleField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  jdouble value = env->GetStaticDoubleField(NULL, fieldID);
  printf("value = %Lf\n", (double)value);

  return TestResult::PASS("GetStaticDoubleField(clazz == NULL) do not crush");

}
