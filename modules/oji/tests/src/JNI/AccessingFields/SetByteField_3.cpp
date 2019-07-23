



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetByteField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_byte", "B");

  env->SetByteField(obj, (jfieldID)100, 1);
  printf("value = %d\n", (int)env->GetByteField(obj, fieldID));
  if(env->GetByteField(obj, fieldID) == 0){
    return TestResult::PASS("SetByteField(fieldID = (jfieldID)100) do not set field - correct");
  }else{
    return TestResult::FAIL("SetByteField(fieldID = (jfieldID)100) set field - incorrect");
  }

}
