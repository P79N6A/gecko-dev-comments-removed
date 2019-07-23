



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetFloatField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");

  env->SetFloatField(NULL, fieldID, 1);
  printf("value = %d\n", (int)env->GetFloatField(obj, fieldID));
  if(env->GetFloatField(obj, fieldID) == 0){
    return TestResult::PASS("SetFloatField(obj == NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetFloatField(obj == NULL) set field - incorrect");
  }

}
