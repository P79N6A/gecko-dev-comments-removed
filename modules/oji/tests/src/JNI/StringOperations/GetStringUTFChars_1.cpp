



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringUTFChars_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF("12345678"); 
  env->CallVoidMethod(obj, MethodID, str);
  char* str_chars = (char *) env->GetStringUTFChars(str, NULL);
  printf("The GetStringUTFChars returned : %s\n\n", str_chars);
  if(str_chars!=NULL){
     return TestResult::PASS("GetStringUTFChars returns correct value (non-empty string and isCopy = NULL)");
  }else{
     return TestResult::FAIL("GetStringUTFChars returns incorrect value (non-empty string and isCopy = NULL)");
  }

}
