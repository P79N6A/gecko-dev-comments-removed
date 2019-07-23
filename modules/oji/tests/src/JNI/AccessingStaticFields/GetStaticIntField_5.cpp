



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticIntField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "I");
  env->SetStaticIntField(clazz, fieldID, MIN_JINT);
  jint value = env->GetStaticIntField(clazz, fieldID);
  printf("value = %d\n", (int)value);
  if(value==MIN_JINT){
     return TestResult::PASS("GetStaticIntField with val == MIN_JINT return correct value");
  }else{
     return TestResult::FAIL("GetStaticIntField with val == MIN_JINT return incorrect value");
  }


}
