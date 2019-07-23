



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetShortField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_short", "S");
  env->SetShortField(obj, fieldID, MIN_JSHORT);
  jshort value = env->GetShortField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MIN_JSHORT){
     return TestResult::PASS("GetShortField with val == MIN_JSHORT return correct value");
  }else{
     return TestResult::FAIL("GetShortField with val == MIN_JSHORT return incorrect value");
  }

}
