



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_10)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_double", "D");
  env->SetDoubleField(obj, fieldID, 10);
  jdouble value = env->GetDoubleField(obj, fieldID);
  if((fieldID != NULL) &&(value == 10)){
    return TestResult::PASS("GetFieldID(all right for double) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for double) failed");
  }

}
