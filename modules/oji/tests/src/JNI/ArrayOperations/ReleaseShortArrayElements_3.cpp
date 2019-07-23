



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseShortArrayElements_3)
{
  GET_JNI_FOR_TEST


  jshortArray arr = env->NewShortArray(3);
  jsize start = 0;
  jsize leng = 3;
  jshort buf[3];
  buf[0] = MAX_JSHORT;
  buf[1] = MIN_JSHORT;
  buf[2] = 10;
  env->SetShortArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jshort *val = env->GetShortArrayElements(arr, &isCopy);
  jshort val0 = val[0];
  jshort val1 = val[1];
  jshort val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 2;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseShortArrayElements(arr, val, JNI_COMMIT);
  jshort *valu = env->GetShortArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==0) && (valu[1]==1) && (valu[2]==2) && (val[0]==0)){
     return TestResult::PASS("ReleaseShortArrayElements(arr, val, JNI_COMMIT) is correct");
  }else{
     return TestResult::FAIL("ReleaseShortArrayElements(arr, val, JNI_COMMIT) is incorrect");
  }
}