



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_ThrowNew_4)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/String;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->ThrowNew(clazz, "test message");
  env->ExceptionDescribe();
  jthrowable excep  = env->ExceptionOccurred();
  if((excep == NULL) && (res <= 0)){
    return TestResult::PASS("ThrowNew(java.lang.String, message not empty) returns correct value and not thrown any exception");
  }else{
    return TestResult::FAIL("ThrowNew(java.lang.String, message not empty) thrown exception");
  }
}