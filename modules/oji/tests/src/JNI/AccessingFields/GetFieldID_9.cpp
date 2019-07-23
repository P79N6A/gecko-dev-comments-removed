



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_9)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_long", "J");
  env->SetLongField(obj, fieldID, 10);
  jlong value = env->GetLongField(obj, fieldID);
  if((fieldID != NULL) &&((int)value == 10)){
    return TestResult::PASS("GetFieldID(all right for long) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for long) failed");
  }

}
