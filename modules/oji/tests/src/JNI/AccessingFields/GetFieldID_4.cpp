



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_int", "");

  if(fieldID == NULL){
    return TestResult::PASS("GetFieldID(sig = empty) passed");
  }else{
    return TestResult::FAIL("GetFieldID(sig = empty) failed");
  }

}
