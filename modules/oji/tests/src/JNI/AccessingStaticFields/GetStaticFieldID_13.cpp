



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_13)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_float", "F");
  env->SetStaticFloatField(clazz, fieldID, 10);
  jfloat value = env->GetStaticFloatField(clazz, fieldID);
  if(value == 10){
    return TestResult::PASS("GetStaticFieldID(all right for float) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for float) return incorrect value");
  }

}
