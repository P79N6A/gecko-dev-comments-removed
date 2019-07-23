



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetIntArrayRegion_4)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(3);
  jsize start = -1;
  jsize leng = 3;
  jint buf[3];
  buf[0] = MAX_JINT;
  buf[1] = MIN_JINT;
  buf[2] = 0;
  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  env->SetIntArrayRegion(arr, start, leng, buf);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz)){
    printf("ArrayIndexOutOfBoundsException is thrown. It is correct!\n");
    return TestResult::PASS("SetIntArrayRegion(where start is -1) returns correct value");
  }else{
    return TestResult::FAIL("SetIntArrayRegion(where start is -1) does not thrown ArrayIndexOutOfBoundsException");
  }
}