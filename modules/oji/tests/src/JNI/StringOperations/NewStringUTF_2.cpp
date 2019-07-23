



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_NewStringUTF_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF(""); 
  env->CallVoidMethod(obj, MethodID, str);
  jsize len = env->GetStringUTFLength(str);
  if((int)len == 0){
     return TestResult::PASS("NewStringUTF(\"\") return correct value");
  }else{
     return TestResult::FAIL("NewStringUTF(\"\") return incorrect value");
  }
}
