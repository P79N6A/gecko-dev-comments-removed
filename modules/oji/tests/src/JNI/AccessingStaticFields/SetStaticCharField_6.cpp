



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticCharField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  env->SetStaticCharField(clazz, fieldID, 'a');
  printf("value = %c\n", (char)env->GetStaticCharField(clazz, fieldID));
  if(env->GetStaticCharField(clazz, fieldID) == (jchar)'a'){
     return TestResult::PASS("SetStaticCharField(all correct, value == a) set correct value to field");
  }else{
     return TestResult::FAIL("SetStaticCharField(all correct, value == a) set incorrect value to field");
  }

}
