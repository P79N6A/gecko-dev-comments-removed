



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticShortField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_short", "S");
  env->SetStaticShortField(clazz, fieldID, MAX_JSHORT);
  jshort value = env->GetStaticShortField(clazz, fieldID);
  printf("value = %d\n", (short)value);
  if(value == MAX_JSHORT){
     return TestResult::PASS("GetStaticShortField with val == MAX_JSHORT return correct value");
  }else{
     return TestResult::FAIL("GetStaticShortField with val == MAX_JSHORT return incorrect value");
  }


}
