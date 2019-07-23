



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetBooleanField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");

  env->SetBooleanField(NULL, fieldID, TRUE);
  printf("value = %d\n", (int)env->GetBooleanField(obj, fieldID));
  if(env->GetBooleanField(obj, fieldID) == 0){
    return TestResult::PASS("SetBooleanField(obj = NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetBooleanField(obj = NULL) set field - incorrect");
  }

}
