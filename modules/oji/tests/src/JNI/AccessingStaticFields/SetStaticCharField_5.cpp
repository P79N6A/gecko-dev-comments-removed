



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticCharField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  env->SetStaticCharField(clazz, fieldID, (jchar)NULL);
  printf("value = %c\n", (char)env->GetStaticCharField(clazz, fieldID));
  if(env->GetStaticCharField(clazz, fieldID) == (jchar)NULL){
     return TestResult::PASS("SetStaticCharField(all correct, value == (jchar)NULL) set correct value to field");
  }else{
     return TestResult::FAIL("SetStaticCharField(all correct, value == (jchar)NULL) set incorrect value to field");
  }
}
