



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringChars_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jchar str_chars[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars, 0); 

  jboolean *isCopy;
  const jchar* chars = env->GetStringChars(str, isCopy);
  if(chars!=NULL){
    return TestResult::PASS("GetStringChars(empty, isCopy) returns correct value");
  }else{
    return TestResult::FAIL("GetStringChars(empty, isCopy) returns incorrect value");
  }
  

}
