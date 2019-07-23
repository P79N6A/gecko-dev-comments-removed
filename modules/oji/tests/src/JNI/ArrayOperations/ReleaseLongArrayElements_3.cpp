



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseLongArrayElements_3)
{
  GET_JNI_FOR_TEST


  jlongArray arr = env->NewLongArray(3);
  jsize start = 0;
  jsize leng = 3;
  jlong buf[3];
  buf[0] = MAX_JLONG;
  buf[1] = MIN_JLONG;
  buf[2] = 10;
  env->SetLongArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jlong *val = env->GetLongArrayElements(arr, &isCopy);
  jlong val0 = val[0];
  jlong val1 = val[1];
  jlong val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 2;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseLongArrayElements(arr, val, JNI_COMMIT);
  jlong *valu = env->GetLongArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==0) && (valu[1]==1) && (valu[2]==2) && (val[0]==0)){
     return TestResult::PASS("ReleaseLongArrayElements(arr, val, JNI_COMMIT) is correct");
  }else{
     return TestResult::FAIL("ReleaseLongArrayElements(arr, val, JNI_COMMIT) is incorrect");
  }
}