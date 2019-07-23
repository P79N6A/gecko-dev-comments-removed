



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetByteField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_byte", "B");

  env->SetByteField(obj, fieldID, (jbyte)-1);
  printf("value = %d\n", (int)env->GetByteField(obj, fieldID));
  if(env->GetByteField(obj, fieldID) == -1){
    return TestResult::PASS("SetByteField(all correct, value = 1) set value to field - correct");
  }else{
    return TestResult::FAIL("SetByteField(all correct, value = 1) do not set value to field - incorrect");
  }

}
