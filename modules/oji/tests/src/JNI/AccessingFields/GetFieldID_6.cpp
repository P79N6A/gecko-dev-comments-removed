



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_byte", "B");
  env->SetByteField(obj, fieldID, 10);
  jbyte value = env->GetByteField(obj, fieldID);
  if((fieldID != NULL) &&((int)value == 10)){
    return TestResult::PASS("GetFieldID(all right for byte) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for byte) failed");
  }

}
