



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetByteField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_byte", "B");
  env->SetByteField(obj, fieldID, MIN_JBYTE);
  jbyte value = env->GetByteField(obj, fieldID);
  printf("value = %d\n", (int)value);
  if(value == MIN_JBYTE){
     return TestResult::PASS("GetByteField with val == MIN_JBYTE return correct value");
  }else{
     return TestResult::FAIL("GetByteField with val == MIN_JBYTE return incorrect value");
  }

}
