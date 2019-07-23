



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_NewString_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jchar str_chars[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars, 0); 
  env->CallVoidMethod(obj, MethodID, str);
  if(str==NULL){
    return TestResult::FAIL("NewString(correct, 0) returns incorrect value");
  }else{
    return TestResult::PASS("NewString(correct, 0) returns correct value");
  }

}
