



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticCharField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  env->SetStaticCharField(clazz, fieldID, 'a');
  jchar value = env->GetStaticCharField(clazz, fieldID);
  printf("value = %c\n", (char)value);
  if(value == 'a'){
     return TestResult::PASS("GetStaticCharField(all correct) return correct value");
  }else{
     return TestResult::FAIL("GetStaticCharField(all correct) return incorrect value");
  }
}
