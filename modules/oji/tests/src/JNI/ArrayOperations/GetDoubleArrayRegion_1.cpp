



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetDoubleArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(3);
  jsize start = 0;
  jsize leng = 3;
  jdouble buf[3];
  buf[0] = MAX_JDOUBLE;
  buf[1] = MIN_JDOUBLE;
  buf[2] = 0;
  env->SetDoubleArrayRegion(arr, start, leng, buf);

  jdouble val[3];
  env->GetDoubleArrayRegion(arr, start, leng, val);

  if((val[0]==MAX_JDOUBLE)&&(val[1]==MIN_JDOUBLE)&&(val[2]==0)){
     return TestResult::PASS("GetDoubleArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetDoubleArrayRegion(all right) returns incorrect value");
  }
}