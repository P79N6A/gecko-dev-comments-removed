



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetByteField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_byte", "B");

  env->SetByteField(NULL, fieldID,1);
  printf("value = %d\n", (int)env->GetByteField(obj, fieldID));
  if(env->GetByteField(obj, fieldID) == 0){
    return TestResult::PASS("SetByteField(obj = NULL) do not set field - correct");
  }else{
    return TestResult::FAIL("SetByteField(obj = NULL) set field - incorrect");
  }

}
