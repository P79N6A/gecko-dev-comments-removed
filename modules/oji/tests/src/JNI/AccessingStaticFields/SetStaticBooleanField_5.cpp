



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticBooleanField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  env->SetStaticBooleanField(clazz, fieldID, JNI_FALSE);
  printf("value = %d\n", (int)env->GetStaticBooleanField(clazz, fieldID));
  if(!env->GetStaticBooleanField(clazz, fieldID)){
     return TestResult::PASS("SetStaticBooleanField(all correct, value == JNI_FALSE) set correct value to field");
  }else{
     return TestResult::FAIL("SetStaticBooleanField(all correct, value == JNI_FALSE) set incorrect value to field");
  }
}
