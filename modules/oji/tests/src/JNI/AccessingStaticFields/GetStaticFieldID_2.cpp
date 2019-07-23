



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", NULL, "I");

  if(fieldID == 0){
     return TestResult::PASS("GetStaticFieldID with param_name == NULL return correct value");
  }else{
     return TestResult::FAIL("GetStaticFieldID with param_name == NULL return incorrect value");
  }

}
