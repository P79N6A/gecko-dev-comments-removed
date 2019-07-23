



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetIntArrayRegion_3)
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
  env->GetIntArrayRegion(NULL, start, leng, val);

  if((val[0]==0)&&(val[1]==0)&&(val[2]==0)){
     return TestResult::PASS("GetIntArrayRegion(arr = NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetIntArrayRegion(arr = NULL) returns incorrect value");
  }
}