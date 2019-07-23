



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_NewString_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jstring str = env->NewString(NULL, 0); 
  env->CallVoidMethod(obj, MethodID, str);
  if(str==NULL){
    return TestResult::PASS("NewString(NULL, 0) returns correct value");
  }else{
    return TestResult::FAIL("NewString(NULL, 0) returns incorrect value");
  }
  
}
