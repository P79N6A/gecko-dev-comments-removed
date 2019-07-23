



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetBooleanField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");

  env->SetBooleanField(obj, fieldID, JNI_FALSE);
  printf("value = %d\n", (int)env->GetBooleanField(obj, fieldID));
  if(env->GetBooleanField(obj, fieldID) == JNI_FALSE){
    return TestResult::PASS("SetBooleanField(all correct, value = JNI_FALSE) set value to field - correct");
  }else{
    return TestResult::FAIL("SetBooleanField(all correct, value = JNI_FALSE) do not set value to field - incorrect");
  }

}
