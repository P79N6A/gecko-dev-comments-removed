



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_nameint", "I");
  if(fieldID == 0){
     return TestResult::PASS("GetStaticFieldID with incorrect name return correct value");
  }else{
     return TestResult::FAIL("GetStaticFieldID with incorrect name return incorrect value");
  }


}
