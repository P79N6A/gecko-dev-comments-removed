



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetLongField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_long", "J");
  env->SetLongField(obj, fieldID, MIN_JLONG);
  jlong value = env->GetLongField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MIN_JLONG){
     return TestResult::PASS("GetLongField with val == MIN_JLONG return correct value");
  }else{
     return TestResult::FAIL("GetLongField with val == MIN_JLONG return incorrect value");
  }

}
