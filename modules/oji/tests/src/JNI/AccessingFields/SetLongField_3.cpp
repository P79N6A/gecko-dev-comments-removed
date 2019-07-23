



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetLongField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");

  env->SetLongField(obj, (jfieldID)100, 1);
  printf("value = %ld\n", env->GetLongField(obj, fieldID));
  if(env->GetLongField(obj, fieldID) == 0){
    return TestResult::PASS("SetLongField(fieldID == (fieldID)100) do not set field - correct");
  }else{
    return TestResult::FAIL("SetLongField(fieldID == (fieldID)100) set field - incorrect");
  }

}
