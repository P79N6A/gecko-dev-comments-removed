



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetCharField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");

  env->SetCharField(obj, NULL, 'a');
  printf("value = %c\n", (char)env->GetCharField(obj, fieldID));
  if(env->GetCharField(obj, fieldID) == 0){
    return TestResult::PASS("SetCharField(fieldID = NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetCharField(fieldID = NULL) set field - incorrect");
  }

}
