



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetIntArrayRegion_1)
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
  env->GetIntArrayRegion(arr, start, leng, val);

  if((val[0]==MAX_JINT)&&(val[1]==MIN_JINT)&&(val[2]==0)){
     return TestResult::PASS("GetIntArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetIntArrayRegion(all right) returns incorrect value");
  }
}