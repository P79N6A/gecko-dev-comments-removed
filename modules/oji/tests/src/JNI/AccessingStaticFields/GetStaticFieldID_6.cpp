



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_final_int", "I");
  jint value = env->GetStaticIntField(clazz, fieldID);
  if((fieldID != NULL) &&((int)value == 11)){
    return TestResult::PASS("GetFieldID(all right for static final int) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for static final int) failed");
  }

}
