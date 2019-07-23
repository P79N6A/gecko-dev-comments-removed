



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetDoubleField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_double", "D");
  env->SetDoubleField(obj, fieldID, MIN_JDOUBLE);
  jdouble value = env->GetDoubleField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MIN_JDOUBLE){
     return TestResult::PASS("GetDoubleField with val == MIN_JDOUBLE return correct value");
  }else{
     return TestResult::FAIL("GetDoubleField with val == MIN_JDOUBLE return incorrect value");
  }

}
