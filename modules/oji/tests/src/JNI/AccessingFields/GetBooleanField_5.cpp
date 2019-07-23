



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetBooleanField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_bool", "Z");
  env->SetBooleanField(obj, fieldID, JNI_TRUE);
  jboolean value = env->GetBooleanField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == JNI_TRUE){
     return TestResult::PASS("GetBooleanField with val == JNI_TRUE return correct value");
  }else{
     return TestResult::FAIL("GetBooleanField with val == JNI_TRUE return incorrect value");
  }

}
