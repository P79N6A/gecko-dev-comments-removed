



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", NULL, "I");

  if(fieldID == NULL){
    return TestResult::PASS("GetFieldID(name_of_param= NULL) passed");
  }else{
    return TestResult::FAIL("GetFieldID(name_of_param= NULL) failed");
  }

}
