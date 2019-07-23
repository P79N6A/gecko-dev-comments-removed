



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetFloatField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");

  env->SetFloatField(obj, fieldID, 1);
  printf("value = %d\n", (int)env->GetFloatField(obj, fieldID));
  if(env->GetFloatField(obj, fieldID) == 1){
    return TestResult::PASS("SetFloatField(all correct) set value to field - correct");
  }else{
    return TestResult::FAIL("SetFloatField(all correct) do not set value to field - incorrect");
  }

}
