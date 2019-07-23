



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetLongField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");
  env->SetLongField(obj, fieldID, MAX_JLONG);
  jlong value = env->GetLongField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MAX_JLONG){
     return TestResult::PASS("GetLongField with val == MAX_JLONG return correct value");
  }else{
     return TestResult::FAIL("GetLongField with val == MAX_JLONG return incorrect value");
  }

}
