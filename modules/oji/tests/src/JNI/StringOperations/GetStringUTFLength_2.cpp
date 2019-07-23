



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringUTFLength_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Set_int_field", "(I)V");
  jstring str = env->NewStringUTF(""); 
  jsize str_size = env->GetStringUTFLength(str);
  env->CallVoidMethod(obj, MethodID, str_size);
  if((int)str_size == 0){
     return TestResult::PASS("GetStringUTFLength(empty string) return correct value!");
  }else{
     return TestResult::FAIL("GetStringUTFLength(empty string) return incorrect value!");
  }

}
