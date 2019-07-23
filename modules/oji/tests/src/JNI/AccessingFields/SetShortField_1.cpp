



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetShortField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");

  env->SetShortField(NULL, fieldID, 1);
  printf("value = %d\n", env->GetShortField(obj, fieldID));
  if(env->GetShortField(obj, fieldID) == 0){
    return TestResult::PASS("SetShortField(obj == NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetShortField(obj == NULL) set field - incorrect");
  }

}
