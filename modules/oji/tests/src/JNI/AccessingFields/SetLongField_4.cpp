



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetLongField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");

  env->SetLongField(obj, NULL, 1);
  printf("value = %ld\n", env->GetLongField(obj, fieldID));
  if(env->GetLongField(obj, fieldID) == 0){
    return TestResult::PASS("SetLongField(fieldID == NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetLongField(fieldID == NULL) set field - incorrect");
  }

}
