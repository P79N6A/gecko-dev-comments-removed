



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetIntField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_int", "I");

  env->SetIntField(obj, (jfieldID)100, 1);
  printf("value = %d\n", (int)env->GetIntField(obj, fieldID));
  if(env->GetIntField(obj, fieldID) == 0){
    return TestResult::PASS("SetIntField(fieldID = (jfieldID)100) do not set field - correct");
  }else{
    return TestResult::FAIL("SetIntField(fieldID = (jfieldID)100) set field - incorrect");
  }

}
