



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_11)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_float", "F");
  env->SetFloatField(obj, fieldID, 10);
  jfloat value = env->GetFloatField(obj, fieldID);
  if((fieldID != NULL) &&(value == 10)){
    return TestResult::PASS("GetFieldID(all right for float) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for float) failed");
  }

}
