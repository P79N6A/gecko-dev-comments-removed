



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticByteField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_byte", "B");
  env->SetStaticByteField(clazz, fieldID, MAX_JBYTE);
  jbyte value = env->GetStaticByteField(clazz, fieldID);
  printf("value = %d\n", (byte)value);
  if(value == MAX_JBYTE){
     return TestResult::PASS("GetStaticByteField with val == MAX_JBYTE return correct value");
  }else{
     return TestResult::FAIL("GetStaticByteField with val == MAX_JBYTE return incorrect value");
  }

}
