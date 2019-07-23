



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_10)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_long", "J");
  env->SetStaticLongField(clazz, fieldID, 10);
  jlong value = env->GetStaticLongField(clazz, fieldID);
  if(value == 10){
    return TestResult::PASS("GetStaticFieldID(all right for long) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for long) return incorrect value");
  }

}
