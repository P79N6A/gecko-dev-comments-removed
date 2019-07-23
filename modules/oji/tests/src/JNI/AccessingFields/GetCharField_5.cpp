



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetCharField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_char", "C");

  env->SetCharField(obj, fieldID, 'a');
  jchar value = env->GetCharField(obj, fieldID);
  printf("value = %c\n", (char)value);
  if(value == 'a'){
     return TestResult::PASS("GetCharField(all correct) return correct value");
  }else{
     return TestResult::FAIL("GetCharField(all correct) return incorrect value");
  }

}
