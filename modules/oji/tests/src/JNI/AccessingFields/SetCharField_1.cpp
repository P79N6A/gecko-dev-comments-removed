



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetCharField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");

  env->SetCharField(NULL, fieldID, 'a');
  printf("value = %c\n", (char)env->GetCharField(obj, fieldID));
  if(env->GetCharField(obj, fieldID) == 0){
    return TestResult::PASS("SetCharField(obi = NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetCharField(obj = NULL) set field - incorrect");
  }

}
