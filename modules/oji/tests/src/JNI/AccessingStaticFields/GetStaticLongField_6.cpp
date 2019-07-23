



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticLongField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  env->SetStaticLongField(clazz, fieldID, MAX_JLONG);
  jlong value = env->GetStaticLongField(clazz, fieldID);
  printf("value = %d\n", (int)value);
  if(value==MAX_JLONG){
     return TestResult::PASS("GetStaticLongField with val == MAX_JLONG return correct value");
  }else{
     return TestResult::FAIL("GetStaticLongField with val == MAX_JLONG return incorrect value");
  }


}
