



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetShortField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");

  env->SetShortField(obj, fieldID, 1);
  printf("value = %d\n", env->GetShortField(obj, fieldID));
  if(env->GetShortField(obj, fieldID) == 1){
    return TestResult::PASS("SetShortField(all correct) set value to field - correct");
  }else{
    return TestResult::FAIL("SetShortField(all correct) do not set value to field - incorrect");
  }

}
