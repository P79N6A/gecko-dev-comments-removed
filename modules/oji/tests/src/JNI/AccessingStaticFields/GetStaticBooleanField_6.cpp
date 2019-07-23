



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticBooleanField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  env->SetStaticBooleanField(clazz, fieldID, JNI_FALSE);
  jboolean value = env->GetStaticBooleanField(clazz, fieldID);
  printf("value = %d\n", (int)value);
  if(value == JNI_FALSE){
    return TestResult::PASS("GetStaticBooleanField with val == JNI_FALSE return correct value");
  }else{
    return TestResult::FAIL("GetStaticBooleanField with val == JNI_FALSE return incorrect value");
  }

}
