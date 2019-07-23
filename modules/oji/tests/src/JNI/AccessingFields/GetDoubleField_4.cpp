



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetDoubleField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_double", "D");
  jdouble value = env->GetDoubleField(obj, (jfieldID)100);
  printf("value = %Lf\n", (double)value);
  return TestResult::PASS("GetDoubleField with fieldID = (jfieldID)100 return correct value - do not crash");

}
