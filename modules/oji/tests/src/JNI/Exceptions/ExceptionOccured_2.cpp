



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_ExceptionOccurred_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jthrowable obj = (jthrowable)env->AllocObject(clazz);
 
  jthrowable excep  = env->ExceptionOccurred();
  if(excep==NULL){
    return TestResult::PASS("ExceptionOccurred(no exception being thrown) is correct");
  }else{
    return TestResult::FAIL("ExceptionOccurred(no exception being thrown) does not correct");
  }
}