



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticShortField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  env->SetStaticShortField(clazz, fieldID, 1);
  printf("value = %d\n", env->GetStaticShortField(clazz, fieldID));
  if(env->GetStaticShortField(clazz, fieldID) == 1){
     return TestResult::PASS("SetStaticShortField(all right, value == 1) return correct value");
  }else{
     return TestResult::FAIL("SetStaticShortField(all right, value == 1) return incorrect value");
  }

}
