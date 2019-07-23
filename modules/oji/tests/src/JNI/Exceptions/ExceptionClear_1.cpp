



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_ExceptionClear_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
  jint res = env->Throw(obj);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz)){
    printf("ExceptionOccurred returns not null value");
  }
  env->ExceptionClear();
  excep = env->ExceptionOccurred();
  if(excep==NULL){
    return TestResult::PASS("ExceptionClear(exception being thrown) really clear exception!");
  }else{
    return TestResult::FAIL("ExceptionClear(exception being thrown) do not clear exception!");
  }
}
