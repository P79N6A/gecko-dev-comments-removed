



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_19)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_int_not_exist", "I");

  if(fieldID == NULL){
    return TestResult::PASS("GetFieldID(with incorrect name of field) passed");
  }else{
    return TestResult::FAIL("GetFieldID(with incorrect name of field) failed");
  }

}
