



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringUTFLength_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Set_int_field", "(I)V");
  jstring str = env->NewStringUTF("12345678"); 
  jsize str_size = env->GetStringUTFLength(str);
  env->CallVoidMethod(obj, MethodID, str_size);
  if((int)str_size == 8){
     return TestResult::PASS("GetStringUTFLength(correct string) return correct value!");
  }else{
     return TestResult::FAIL("GetStringUTFLength(correct string) return incorrect value!");
  }
  
}
