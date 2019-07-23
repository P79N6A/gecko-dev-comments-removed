



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetIntArrayRegion_4)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(3);
  jsize start = 0;
  jsize leng = 3;
  jint buf[3];
  buf[0] = MAX_JINT;
  buf[1] = MIN_JINT;
  buf[2] = 0;
  jclass clazz = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  env->SetIntArrayRegion(arr, start, leng, buf);
  start = -1;
  jint val[3];
  env->GetIntArrayRegion(arr, start, leng, val);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz)){
    printf("ArrayIndexOutOfBoundsException is thrown. It is correct!\n");
    return TestResult::PASS("GetIntArrayRegion(where start is -1) returns correct value");
  }else{
    return TestResult::FAIL("GetIntArrayRegion(where start is -1) does not thrown ArrayIndexOutOfBoundsException");
  }
}