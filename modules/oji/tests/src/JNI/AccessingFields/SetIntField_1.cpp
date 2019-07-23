



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetIntField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_int", "I");

  env->SetIntField(NULL, fieldID, 1);
  printf("value = %d\n", (int)env->GetIntField(obj, fieldID));
  if(env->GetIntField(obj, fieldID) == 0){
    return TestResult::PASS("SetIntField(obj == NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetIntField(obj == NULL) set field - incorrect");
  }

}
