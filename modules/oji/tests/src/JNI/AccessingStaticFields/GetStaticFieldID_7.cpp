



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_7)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_bool", "Z");
  jboolean value = env->GetStaticBooleanField(clazz, fieldID);
  if(value == JNI_TRUE){
    return TestResult::PASS("GetStaticFieldID(all right for boolean) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for boolean) return incorrect value");
  }

}
