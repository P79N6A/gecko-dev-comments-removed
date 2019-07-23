



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetBooleanField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");
  env->SetBooleanField(obj, fieldID, JNI_FALSE);
  jboolean value = env->GetBooleanField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == JNI_FALSE){
     return TestResult::PASS("GetBooleanField with val == JNI_FALSE return correct value");
  }else{
     return TestResult::FAIL("GetBooleanField with val == JNI_FALSE return incorrect value");
  }

}
