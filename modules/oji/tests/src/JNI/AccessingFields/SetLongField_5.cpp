



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetLongField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");

  env->SetLongField(obj, fieldID, 1);
  printf("value = %ld\n", env->GetLongField(obj, fieldID));
  if(env->GetLongField(obj, fieldID) == 1){
    return TestResult::PASS("SetLongField(all correct) set value to field - correct");
  }else{
    return TestResult::FAIL("SetLongField(all correct) do not set value to field - incorrect");
  }

}
