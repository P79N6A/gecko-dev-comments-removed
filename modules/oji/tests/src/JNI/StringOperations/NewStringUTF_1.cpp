



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_NewStringUTF_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF("12345678"); 
  env->CallVoidMethod(obj, MethodID, str);

  jsize len = env->GetStringUTFLength(str);
  if((int)len == 8){
     return TestResult::PASS("NewStringUTF(non empty string) return correct value");
  }else{
     return TestResult::FAIL("NewStringUTF(non empty string) return incorrect value");
  }

}
