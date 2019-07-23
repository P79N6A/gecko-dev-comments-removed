



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticLongField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  env->SetStaticLongField(clazz, fieldID, MIN_JLONG);
  jlong value = env->GetStaticLongField(clazz, fieldID);
  printf("value = %d\n", (int)value);
  if(value==MIN_JLONG){
     return TestResult::PASS("GetStaticLongField with val == MIN_JLONG return correct value");
  }else{
     return TestResult::FAIL("GetStaticLongField with val == MIN_JLONG return incorrect value");
  }


}
