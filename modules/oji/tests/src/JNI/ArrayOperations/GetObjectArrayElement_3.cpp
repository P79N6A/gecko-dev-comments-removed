



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetObjectArrayElement_3)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jobjectArray arr = env->NewObjectArray(4, clazz, NULL);
  jstring str_n = (jstring)env->GetObjectArrayElement(NULL, (jsize) 0);
  if(str_n == NULL){
     return TestResult::PASS("NewObjectArray(array as NULL) returns correct value - empty array");
  }else{
     return TestResult::FAIL("NewObjectArray(array as NULL) returns incorrect value");
  }


}
