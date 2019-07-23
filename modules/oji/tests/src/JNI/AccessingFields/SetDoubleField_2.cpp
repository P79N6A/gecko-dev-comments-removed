



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetDoubleField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_double", "D");

  env->SetDoubleField(obj, (jfieldID)-100, 1);
  printf("value = %Lf\n", env->GetDoubleField(obj, fieldID));
  if(env->GetDoubleField(obj, fieldID) == 0){
    return TestResult::PASS("SetDoubleField(fieldID = (jfieldID)-100) do not set field - correct");
  }else{
    return TestResult::FAIL("SetDoubleField(fieldID = (jfieldID)-100) set field - incorrect");
  }

}
