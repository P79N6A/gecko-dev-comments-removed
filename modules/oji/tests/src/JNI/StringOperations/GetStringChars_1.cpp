



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringChars_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jboolean *isCopy;
  const jchar* chars = env->GetStringChars(NULL, isCopy);
  if(chars==NULL){
    return TestResult::PASS("GetStringChars(NULL, isCopy) returns correct value");
  }else{
    return TestResult::FAIL("GetStringChars(NULL, isCopy) returns incorrect value");
  }
  

}
