



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetIntArrayRegion_2)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(3);
  jsize start = 0;
  jsize leng = 3;
  jint buf[3];
  buf[0] = MAX_JINT;
  buf[1] = MIN_JINT;
  buf[2] = 0;
  env->SetIntArrayRegion(arr, start, leng, buf);

  jint val[3];
  env->GetIntArrayRegion(arr, start, leng, NULL);

  return TestResult::PASS("GetIntArrayRegion(with buf as NULL) returns correct value");
}