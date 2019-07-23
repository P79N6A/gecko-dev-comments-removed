



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseDoubleArrayElements_1)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(3);
  jsize start = 0;
  jsize leng = 3;
  jdouble buf[3];
  buf[0] = MAX_JDOUBLE;
  buf[1] = MIN_JDOUBLE;
  buf[2] = 10;
  env->SetDoubleArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jdouble *val = env->GetDoubleArrayElements(arr, &isCopy);
  jdouble val0 = val[0];
  jdouble val1 = val[1];
  jdouble val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 2;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseDoubleArrayElements(arr, val, 0);
  jdouble *valu = env->GetDoubleArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==0) && (valu[1]==1) && (valu[2]==2)){
     return TestResult::PASS("ReleaseDoubleArrayElements(arr, val, 0) is correct");
  }else{
     return TestResult::FAIL("ReleaseDoubleArrayElements(arr, val, 0) is incorrect");
  }
}