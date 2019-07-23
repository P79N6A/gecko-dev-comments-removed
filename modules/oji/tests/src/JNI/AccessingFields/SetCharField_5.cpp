



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetCharField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");

  env->SetCharField(obj, fieldID, (jchar)NULL);
  printf("value = %c\n", (char)env->GetCharField(obj, fieldID));
  if(env->GetCharField(obj, fieldID) == (jchar)NULL){
    return TestResult::PASS("SetCharField(all correct, value = NULL) set value to field - correct");
  }else{
    return TestResult::FAIL("SetCharField(all correct, value = NULL) do not set value to field - incorrect");
  }

}
