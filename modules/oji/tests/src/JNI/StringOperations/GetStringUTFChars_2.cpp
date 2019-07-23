



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringUTFChars_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF(NULL); 
  env->CallVoidMethod(obj, MethodID, str);
  jboolean isCopy = TRUE;
  char* str_chars = (char *) env->GetStringUTFChars(str, &isCopy);
  printf("The GetStringUTFChars returned : %s\n\n", str_chars);
  if(str_chars==NULL){
     return TestResult::PASS("GetStringUTFChars returns correct value (string=NULL and isCopy is not NULL)");
  }else{
     return TestResult::FAIL("GetStringUTFChars do not returns correct value (string=NULL and isCopy is not NULL)");
  }

}
