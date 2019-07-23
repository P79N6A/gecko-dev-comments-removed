



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "", "I");

  if(fieldID == NULL){
    return TestResult::PASS("GetFieldID(name_of_param = empty) passed");
  }else{
    return TestResult::FAIL("GetFieldID(name_of_param = empty) failed");
  }

}
