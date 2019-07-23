



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_ThrowNew_3)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->ThrowNew(clazz, NULL);
  env->ExceptionDescribe();
  jthrowable excep  = env->ExceptionOccurred();
  if((env->IsInstanceOf(excep, clazz)) && (res == 0)){
    printf("ArrayIndexOutOfBoundsException is thrown. It is correct!\n");
    return TestResult::PASS("ThrowNew(java.lang.ArrayIndexOutOfBoundsException, mess= NULL) returns correct value and thrown ArrayIndexOutOfBoundsException");
  }else{
    return TestResult::FAIL("ThrowNew(java.lang.ArrayIndexOutOfBoundsException, mess= NULL) does not thrown ArrayIndexOutOfBoundsException");
  }
}