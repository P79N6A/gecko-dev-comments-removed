



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticIntField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int", "I");
  env->SetStaticIntField(clazz, fieldID, 1);
  printf("value = %d\n", (int)env->GetStaticIntField(clazz, fieldID));
  if(env->GetStaticIntField(clazz, fieldID) == 1){
     return TestResult::PASS("SetStaticIntField(all right, value == 1) return correct value");
  }else{
     return TestResult::FAIL("SetStaticIntField(all right, value == 1) return incorrect value");
  }

}
