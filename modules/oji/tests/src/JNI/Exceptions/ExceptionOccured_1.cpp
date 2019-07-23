



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_ExceptionOccurred_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->Throw(obj);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz)){
    return TestResult::PASS("ExceptionOccurred(exception being thrown) is correct");
  }else{
    return TestResult::FAIL("ExceptionOccurred(exception being thrown) does not correct");
  }
}