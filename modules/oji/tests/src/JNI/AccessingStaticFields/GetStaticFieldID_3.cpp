



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", NULL);
  if(fieldID == 0){
     return TestResult::PASS("GetStaticFieldID with sig == NULL return correct value");
  }else{
     return TestResult::FAIL("GetStaticFieldID with sig == NULL return incorrect value");
  }

}
