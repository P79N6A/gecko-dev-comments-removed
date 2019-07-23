



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticBooleanField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  env->SetStaticBooleanField(clazz, fieldID, JNI_TRUE);
  jboolean value = env->GetStaticBooleanField(clazz, fieldID);
  printf("value = %d\n", (int)value);
  if(value == JNI_TRUE){
    return TestResult::PASS("GetStaticBooleanField with val == JNI_TRUE return correct value");
  }else{
    return TestResult::FAIL("GetStaticBooleanField with val == JNI_TRUE return incorrect value");
  }

}
