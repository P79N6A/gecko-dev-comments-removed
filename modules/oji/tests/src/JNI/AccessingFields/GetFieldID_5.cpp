



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_int", NULL);

  if(fieldID == NULL){
    return TestResult::PASS("GetFieldID(sig = NULL) passed");
  }else{
    return TestResult::FAIL("GetFieldID(sig = NULL) failed");
  }

}
