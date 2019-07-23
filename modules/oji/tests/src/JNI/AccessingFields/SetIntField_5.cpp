



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetIntField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_int", "I");

  env->SetIntField(obj, fieldID, 1);
  printf("value = %d\n", (int)env->GetIntField(obj, fieldID));
  if(env->GetIntField(obj, fieldID) == 1){
    return TestResult::PASS("SetIntField(all correct) set value to field - correct");
  }else{
    return TestResult::FAIL("SetIntField(all correct) do not set value to field - incorrect");
  }

}
