



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_8)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_short", "S");
  env->SetShortField(obj, fieldID, 10);
  jshort value = env->GetShortField(obj, fieldID);
  if((fieldID != NULL) &&((int)value == 10)){
    return TestResult::PASS("GetFieldID(all right for short) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for short) failed");
  }

}
