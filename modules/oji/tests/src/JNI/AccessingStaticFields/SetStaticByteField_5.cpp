



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticByteField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  env->SetStaticByteField(clazz, fieldID, (jbyte)10);
  printf("value = %d\n", (int)env->GetStaticByteField(clazz, fieldID));
  if(env->GetStaticByteField(clazz, fieldID) == 10){
    return TestResult::PASS("SetStaticByteField(all correct, value == 10) set correct value to field");
  }else{
    return TestResult::FAIL("SetStaticByteField(all correct, value == 10) set incorrect value to field");
  }
}
