



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetShortField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");

  env->SetShortField(obj, (jfieldID)-100, 1);
  printf("value = %d\n", env->GetShortField(obj, fieldID));
  if(env->GetShortField(obj, fieldID) == 0){
    return TestResult::PASS("SetShortField(with fieldID == (jfieldID)-100) do not set field - correct");
  }else{
    return TestResult::FAIL("SetShortField(with fieldID == (jfieldID)-100) set field - incorrect");
  }
}
