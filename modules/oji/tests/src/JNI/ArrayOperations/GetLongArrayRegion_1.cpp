



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetLongArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jlongArray arr = env->NewLongArray(3);
  jsize start = 0;
  jsize leng = 3;
  jlong buf[3];
  buf[0] = MAX_JLONG;
  buf[1] = MIN_JLONG;
  buf[2] = 0;
  env->SetLongArrayRegion(arr, start, leng, buf);

  jlong val[3];
  env->GetLongArrayRegion(arr, start, leng, val);

  if((val[0]==MAX_JLONG)&&(val[1]==MIN_JLONG)&&(val[2]==0)){
     return TestResult::PASS("GetLongArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetLongArrayRegion(all right) returns incorrect value");
  }
}