



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_ThrowNew_5)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/String;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->ThrowNew(NULL, NULL);
  env->ExceptionDescribe();
  jthrowable excep  = env->ExceptionOccurred();
  if((excep == NULL) && (res < 0)){
    return TestResult::PASS("ThrowNew(NULL, NULL) returns correct value and not thrown any exception");
  }else{
    return TestResult::FAIL("ThrowNew(NULL, NULL) thrown exception");
  }
}