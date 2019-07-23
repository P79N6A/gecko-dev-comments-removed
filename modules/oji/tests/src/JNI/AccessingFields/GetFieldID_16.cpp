



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_16)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_int_super", "I");
  env->SetIntField(obj, fieldID, 10);
  jint value = env->GetIntField(obj, fieldID);
  if((fieldID != NULL) &&((int)value == 10)){
    return TestResult::PASS("GetFieldID(all right for super int) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for super int) failed");
  }

}
