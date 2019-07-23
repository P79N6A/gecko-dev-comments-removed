



































#include "JNIEnvTests.h"
#include "StringOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseStringChars_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string", "(Ljava/lang/String;)V");

  jchar str_chars[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars, 4); 

  jboolean isCopy = JNI_TRUE;
  const jchar* chars = env->GetStringChars(str, &isCopy);
  env->ReleaseStringChars(str, chars);
  return TestResult::PASS("ReleaseStringChars(correct, correct) correct");

}
