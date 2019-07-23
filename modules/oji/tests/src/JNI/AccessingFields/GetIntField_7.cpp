



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetIntField_7)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_int", "I");
  env->SetIntField(obj, fieldID, MAX_JINT);
  jint value = env->GetIntField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MAX_JINT){
     return TestResult::PASS("GetIntField with val == MAX_JINT return correct value");
  }else{
     return TestResult::FAIL("GetIntField with val == MAX_JINT return incorrect value");
  }

}
