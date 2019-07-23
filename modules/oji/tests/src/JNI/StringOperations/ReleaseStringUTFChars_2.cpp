



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseStringUTFChars_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF("12345678"); 
  env->CallVoidMethod(obj, MethodID, str);
  jboolean isCopy = TRUE;
  char* str_chars = (char *) env->GetStringUTFChars(str, &isCopy);
  printf("The GetStringUTFChars returned : %s\n\n", str_chars);
  env->ReleaseStringUTFChars(NULL, NULL);


  return TestResult::PASS("ReleaseStringUTFChars(NULL, NULL) do not failed");

}
