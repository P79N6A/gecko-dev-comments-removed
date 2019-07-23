



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetFloatField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");

  env->SetFloatField(obj, (jfieldID)-100, 1);
  printf("value = %d\n", (int)env->GetFloatField(obj, fieldID));
  if(env->GetFloatField(obj, fieldID) == 0){
    return TestResult::PASS("SetFloatField(fieldID = (jfieldID)-100) do not set field - correct");
  }else{
    return TestResult::FAIL("SetFloatField(fieldID = (jfieldID)-100) set field - incorrect");
  }

}
