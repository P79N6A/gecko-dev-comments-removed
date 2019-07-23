



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFloatField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_float", "F");
  env->SetFloatField(obj, fieldID, MIN_JFLOAT);
  jfloat value = env->GetFloatField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MIN_JFLOAT){
     return TestResult::PASS("GetFloatField with val == MIN_JFLOAT return correct value");
  }else{
     return TestResult::FAIL("GetFloatField with val == MIN_JFLOAT return incorrect value");
  }

}
