



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFloatField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_float", "F");
  env->SetStaticFloatField(clazz, fieldID, MAX_JFLOAT);
  jfloat value = env->GetStaticFloatField(clazz, fieldID);
  printf("value = %f\n", value);
  if(value==MAX_JFLOAT){
     return TestResult::PASS("GetStaticFloatField with val == MAX_JFLOAT return correct value");
  }else{
     return TestResult::FAIL("GetStaticFloatField with val == MAX_JFLOAT return incorrect value");
  }


}
