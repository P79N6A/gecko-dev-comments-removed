



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_20)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "final_int", "I");
  env->SetIntField(obj, fieldID, 10);
  jint value = env->GetIntField(obj, fieldID);
  if((fieldID != NULL) &&((int)value == 10)){
    return TestResult::FAIL("GetFieldID(all right for final int) failed");
  }else{
    return TestResult::PASS("GetFieldID(all right for final int) passed - exception thrown");
  }

}
