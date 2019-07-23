



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetCharField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");

  env->SetCharField(obj, fieldID, 'a');
  printf("value = %c\n", (char)env->GetCharField(obj, fieldID));
  if(env->GetCharField(obj, fieldID) == (jchar)'a'){
    return TestResult::PASS("SetCharField(all correct, value = a) set value to field - correct");
  }else{
    return TestResult::FAIL("SetCharField(all correct, value = a) do not set value to field - incorrect");
  }                                                            
}
