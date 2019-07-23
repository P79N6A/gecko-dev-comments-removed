



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_Throw_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->Throw(NULL);
  jthrowable excep  = env->ExceptionOccurred();
  if((env->IsInstanceOf(excep, clazz)) && (res < 0)){
    return TestResult::FAIL("Throw(NULL) thrown Exception");
  }else{
    printf("ArrayIndexOutOfBoundsException is not thrown. It is correct!\n");
    return TestResult::PASS("Throw(NULL) returns correct value and not thrown any Exceptions");
  }
}