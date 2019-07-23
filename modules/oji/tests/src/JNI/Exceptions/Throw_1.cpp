



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_Throw_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->Throw(obj);
  jthrowable excep  = env->ExceptionOccurred();
  if((env->IsInstanceOf(excep, clazz)) && (res == 0)){
    printf("ArrayIndexOutOfBoundsException is thrown. It is correct!\n");
    return TestResult::PASS("Throw(java.lang.ArrayIndexOutOfBoundsException) returns correct value and thrown ArrayIndexOutOfBoundsException");
  }else{
    return TestResult::FAIL("Throw(java.lang.ArrayIndexOutOfBoundsException) does not thrown ArrayIndexOutOfBoundsException");
  }
}