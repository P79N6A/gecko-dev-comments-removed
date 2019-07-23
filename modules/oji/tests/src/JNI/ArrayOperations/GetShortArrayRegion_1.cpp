



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetShortArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jshortArray arr = env->NewShortArray(3);
  jsize start = 0;
  jsize leng = 3;
  jshort buf[3];
  buf[0] = MAX_JSHORT;
  buf[1] = MIN_JSHORT;
  buf[2] = 0;
  env->SetShortArrayRegion(arr, start, leng, buf);

  jshort val[3];
  env->GetShortArrayRegion(arr, start, leng, val);

  if((val[0]==MAX_JSHORT)&&(val[1]==MIN_JSHORT)&&(val[2]==0)){
     return TestResult::PASS("GetShortArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetShortArrayRegion(all right) returns incorrect value");
  }
}