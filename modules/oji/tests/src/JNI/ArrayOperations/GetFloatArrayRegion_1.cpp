



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetFloatArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jfloatArray arr = env->NewFloatArray(3);
  jsize start = 0;
  jsize leng = 3;
  jfloat buf[3];
  buf[0] = MAX_JFLOAT;
  buf[1] = MIN_JFLOAT;
  buf[2] = 0;
  env->SetFloatArrayRegion(arr, start, leng, buf);

  jfloat val[3];
  env->GetFloatArrayRegion(arr, start, leng, val);

  if((val[0]==MAX_JFLOAT)&&(val[1]==MIN_JFLOAT)&&(val[2]==0)){
     return TestResult::PASS("GetFloatArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetFloatArrayRegion(all right) returns incorrect value");
  }
}