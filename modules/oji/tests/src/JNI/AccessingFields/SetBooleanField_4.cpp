



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetBooleanField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");

  env->SetBooleanField(obj, NULL, TRUE);
  printf("value = %d\n", (int)env->GetBooleanField(obj, fieldID));
  if(env->GetBooleanField(obj, fieldID) == 0){
    return TestResult::PASS("SetBooleanField(fieldID = NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetBooleanField(fieldID = NULL) set field - incorrect");
  }

}
