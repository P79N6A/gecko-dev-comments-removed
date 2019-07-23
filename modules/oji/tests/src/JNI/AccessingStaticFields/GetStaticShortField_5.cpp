



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticShortField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  env->SetStaticShortField(clazz, fieldID, MIN_JSHORT);
  jshort value = env->GetStaticShortField(clazz, fieldID);
  printf("value = %d\n", (short)value);
  if(value == MIN_JSHORT){
     return TestResult::PASS("GetStaticShortField with val == MIN_JSHORT return correct value");
  }else{
     return TestResult::FAIL("GetStaticShortField with val == MIN_JSHORT return incorrect value");
  }


}
