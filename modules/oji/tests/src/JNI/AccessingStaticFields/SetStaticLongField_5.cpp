



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticLongField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  env->SetStaticLongField(clazz, fieldID, 1);
  printf("value = %ld\n", env->GetStaticLongField(clazz, fieldID));
  if(env->GetStaticLongField(clazz, fieldID) == 1){
     return TestResult::PASS("SetStaticLongField(all right, value == 1) return correct value");
  }else{
     return TestResult::FAIL("SetStaticLongField(all right, value == 1) return incorrect value");
  }

}
