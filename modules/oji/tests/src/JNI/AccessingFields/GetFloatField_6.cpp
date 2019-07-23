



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFloatField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");
  env->SetFloatField(obj, fieldID, MAX_JFLOAT);
  jfloat value = env->GetFloatField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MAX_JFLOAT){
     return TestResult::PASS("GetFloatField with val == MAX_JFLOAT return correct value");
  }else{
     return TestResult::FAIL("GetFloatField with val == MAX_JFLOAT return incorrect value");
  }

}
