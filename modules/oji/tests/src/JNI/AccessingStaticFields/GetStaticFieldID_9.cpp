



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_9)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  env->SetStaticShortField(clazz, fieldID, 10);
  jshort value = env->GetStaticShortField(clazz, fieldID);
  if(value == 10){
    return TestResult::PASS("GetStaticFieldID(all right for short) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for short) return incorrect value");
  }

}
