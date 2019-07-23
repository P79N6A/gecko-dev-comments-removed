



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseFloatArrayElements_3)
{
  GET_JNI_FOR_TEST


  jfloatArray arr = env->NewFloatArray(3);
  jsize start = 0;
  jsize leng = 3;
  jfloat buf[3];
  buf[0] = MAX_JFLOAT;
  buf[1] = MIN_JFLOAT;
  buf[2] = 10;
  env->SetFloatArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jfloat *val = env->GetFloatArrayElements(arr, &isCopy);
  jfloat val0 = val[0];
  jfloat val1 = val[1];
  jfloat val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 2;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseFloatArrayElements(arr, val, JNI_COMMIT);
  jfloat *valu = env->GetFloatArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==0) && (valu[1]==1) && (valu[2]==2) && (val[0]==0)){
     return TestResult::PASS("ReleaseFloatArrayElements(arr, val, JNI_COMMIT) is correct");
  }else{
     return TestResult::FAIL("ReleaseFloatArrayElements(arr, val, JNI_COMMIT) is incorrect");
  }
}