



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetShortField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");

  env->SetShortField(obj, NULL, 1);
  printf("value = %d\n", env->GetShortField(obj, fieldID));
  if(env->GetShortField(obj, fieldID) == 0){
    return TestResult::PASS("SetShortField(fieldID == NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetShortField(fieldID == NULL) set field - incorrect");
  }

}
