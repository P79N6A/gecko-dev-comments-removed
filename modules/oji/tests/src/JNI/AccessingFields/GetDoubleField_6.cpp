



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetDoubleField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_double", "D");
  env->SetDoubleField(obj, fieldID, MAX_JDOUBLE);
  jdouble value = env->GetDoubleField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MAX_JDOUBLE){
     return TestResult::PASS("GetDoubleField with val == MAX_JDOUBLE return correct value");
  }else{
     return TestResult::FAIL("GetDoubleField with val == MAX_JDOUBLE return incorrect value");
  }

}
