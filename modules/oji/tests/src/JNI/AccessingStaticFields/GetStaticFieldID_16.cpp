



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_16)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "public_static_super_int", "I");
  jint value = env->GetStaticIntField(clazz, fieldID);
  if(value == 1){
    return TestResult::PASS("GetStaticFieldID(all right for super int) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for super int) return incorrect value");
  }

}
