



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticIntField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "I");
  env->SetStaticIntField(clazz, fieldID, MAX_JINT);
  jint value = env->GetStaticIntField(clazz, fieldID);
  printf("value = %d\n", (int)value);
  if(value==MAX_JINT){
     return TestResult::PASS("GetStaticIntField with val == MAX_JINT return correct value");
  }else{
     return TestResult::FAIL("GetStaticIntField with val == MAX_JINT return incorrect value");
  }


}
