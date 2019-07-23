



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_13)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_bool", "Z");
  env->SetBooleanField(obj, fieldID, JNI_TRUE);
  jboolean value = env->GetBooleanField(obj, fieldID);
  if((fieldID != NULL) &&(value == JNI_TRUE)){
    return TestResult::PASS("GetFieldID(all right for boolean) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for boolean) failed");
  }

}
