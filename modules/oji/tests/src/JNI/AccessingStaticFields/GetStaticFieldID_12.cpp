



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_12)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_char", "C");
  env->SetStaticCharField(clazz, fieldID, 'a');
  jchar value = env->GetStaticCharField(clazz, fieldID);
  if(value == 'a'){
    return TestResult::PASS("GetStaticFieldID(all right for char) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for char) return incorrect value");
  }

}
