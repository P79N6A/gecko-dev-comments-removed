



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_12)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_char", "C");
  env->SetCharField(obj, fieldID, 'a');
  jchar value = env->GetCharField(obj, fieldID);
  if((fieldID != NULL) &&(value == 'a')){
    return TestResult::PASS("GetFieldID(all right for char) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for char) failed");
  }

}
