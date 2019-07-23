



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetBooleanField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");

  env->SetBooleanField(obj, fieldID, TRUE);
  printf("value = %d\n", (int)env->GetBooleanField(obj, fieldID));
  if(env->GetBooleanField(obj, fieldID) == JNI_TRUE){
    return TestResult::PASS("SetBooleanField(all correct, value = JNI_TRUE) set value to field - correct");
  }else{
    return TestResult::FAIL("SetBooleanField(all correct, value = JNI_TRUE) do not set value to field - incorrect");
  }

}
