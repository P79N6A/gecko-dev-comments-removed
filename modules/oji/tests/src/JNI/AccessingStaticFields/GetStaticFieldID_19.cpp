



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_19)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "");
  if(fieldID == 0){
     return TestResult::PASS("GetStaticFieldID with empty sig return correct value");
  }else{
     return TestResult::FAIL("GetStaticFieldID with empty sig return incorrect value");
  }

}
