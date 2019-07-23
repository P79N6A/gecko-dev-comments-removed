



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_18)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "int_field", "I");
  env->SetIntField(obj, fieldID, 10);
  jint value = env->GetIntField(obj, fieldID);
  if((fieldID != NULL) &&((int)value == 10)){
    return TestResult::FAIL("GetFieldID(all right for private int) failed");
  }else{
    return TestResult::PASS("GetFieldID(all right for private int) passed");
  }

}
