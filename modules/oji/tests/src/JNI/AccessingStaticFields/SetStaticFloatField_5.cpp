



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticFloatField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_float", "F");
  env->SetStaticFloatField(clazz, fieldID, 1);
  printf("value = %d\n", (int)env->GetStaticFloatField(clazz, fieldID));
  if(env->GetStaticFloatField(clazz, fieldID) == 1){
     return TestResult::PASS("SetStaticFloatField(all right, value == 1) return correct value");
  }else{
     return TestResult::FAIL("SetStaticFloatField(all right, value == 1) return incorrect value");
  }

}
