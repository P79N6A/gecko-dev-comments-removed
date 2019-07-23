



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_NewStringUTF_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF(NULL); 
  env->CallVoidMethod(obj, MethodID, str);
  if(str!=NULL){
     return TestResult::FAIL("NewStringUTF(NULL) != NULL!");
  }else{
     return TestResult::PASS("NewStringUTF(NULL) == NULL, correct!");
  }

}
