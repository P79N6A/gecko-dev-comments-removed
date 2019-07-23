



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_GetStringLength_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jchar str_chars[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars, 4); 
  env->CallVoidMethod(obj, MethodID, str);

  jsize len = env->GetStringLength(str);
  if((int)len==4){
    return TestResult::PASS("GetStringLength(NULL) returns correct value");
  }else{
    return TestResult::FAIL("GetStringLength(NULL) returns incorrect value");
  }

}
