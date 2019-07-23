



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetObjectArrayElement_2)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jobjectArray arr = env->NewObjectArray(4, clazz, NULL);
  jstring str_n = (jstring)env->GetObjectArrayElement(arr, (jsize) 0);
  if(str_n == NULL){
     return TestResult::PASS("NewObjectArray(object as NULL) returns correct value - empty array");
  }else{
     return TestResult::FAIL("NewObjectArray(object as NULL) returns incorrect value");
  }


}
