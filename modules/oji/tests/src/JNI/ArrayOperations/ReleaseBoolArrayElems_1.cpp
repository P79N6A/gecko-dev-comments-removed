



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseBooleanArrayElements_1)
{
  GET_JNI_FOR_TEST

  jbooleanArray arr = env->NewBooleanArray(3);
  jsize start = 0;
  jsize leng = 3;
  jboolean buf[3];
  buf[0] = JNI_TRUE;
  buf[1] = JNI_FALSE;
  buf[2] = 0;
  env->SetBooleanArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jboolean *val = env->GetBooleanArrayElements(arr, &isCopy);
  jboolean val0 = val[0];
  jboolean val1 = val[1];
  jboolean val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 1;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseBooleanArrayElements(arr, val, 0);
  jboolean *valu = env->GetBooleanArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==0) && (valu[1]==1) && (valu[2]==1)){
     return TestResult::PASS("ReleaseBooleanArrayElements(arr, val, 0) is correct");
  }else{
     return TestResult::FAIL("ReleaseBooleanArrayElements(arr, val, 0) is incorrect");
  }
}