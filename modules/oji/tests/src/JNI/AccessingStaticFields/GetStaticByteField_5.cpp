



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticByteField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  env->SetStaticByteField(clazz, fieldID, MIN_JBYTE);
  jbyte value = env->GetStaticByteField(clazz, fieldID);
  printf("value = %d\n", (byte)value);
  if(value == MIN_JBYTE){
     return TestResult::PASS("GetStaticByteField with val == MIN_JBYTE return correct value");
  }else{
     return TestResult::FAIL("GetStaticByteField with val == MIN_JBYTE return incorrect value");
  }

}
