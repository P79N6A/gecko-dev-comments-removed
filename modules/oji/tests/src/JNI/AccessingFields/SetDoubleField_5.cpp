



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetDoubleField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_double", "D");

  env->SetDoubleField(obj, fieldID, (jdouble)10);
  printf("value = %Lf\n", env->GetDoubleField(obj, fieldID));
  if(env->GetDoubleField(obj, fieldID) == 10){
    return TestResult::PASS("SetDoubleField(all correct) set value to field - correct");
  }else{
    return TestResult::FAIL("SetDoubleField(all correct) do not set value to field - incorrect");
  }

}
