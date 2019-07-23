



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseIntArrayElements_1)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(3);
  jsize start = 0;
  jsize leng = 3;
  jint buf[3];
  buf[0] = MAX_JINT;
  buf[1] = MIN_JINT;
  buf[2] = 10;
  env->SetIntArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jint *val = env->GetIntArrayElements(arr, &isCopy);
  jint val0 = val[0];
  jint val1 = val[1];
  jint val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 2;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseIntArrayElements(arr, val, 0);
  jint *valu = env->GetIntArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==0) && (valu[1]==1) && (valu[2]==2)){
     return TestResult::PASS("ReleaseIntArrayElements(arr, val, 0) is correct");
  }else{
     return TestResult::FAIL("ReleaseIntArrayElements(arr, val, 0) is incorrect");
  }
}