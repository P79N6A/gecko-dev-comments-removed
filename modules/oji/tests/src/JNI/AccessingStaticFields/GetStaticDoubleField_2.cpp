



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticDoubleField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  jdouble value = env->GetStaticDoubleField(clazz, NULL);
  printf("value = %Lf\n", (double)value);

  return TestResult::PASS("GetStaticDoubleField(fieldID == NULL) do not crush");

}
