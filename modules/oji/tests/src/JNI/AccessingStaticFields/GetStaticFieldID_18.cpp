



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_18)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int_private", "I");
  jint value = env->GetStaticIntField(clazz, fieldID);
  if(value == 1){
    return TestResult::FAIL("GetStaticFieldID(all right for private int) return incorrect value");
  }else{
    return TestResult::PASS("GetStaticFieldID(all right for private int) return correct value - exception thrown");
  }

}
