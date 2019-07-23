



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseByteArrayElements_2)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(3);
  jsize start = 0;
  jsize leng = 3;
  jbyte buf[3];
  buf[0] = MAX_JBYTE;
  buf[1] = MIN_JBYTE;
  buf[2] = 10;
  env->SetByteArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jbyte *val = env->GetByteArrayElements(arr, &isCopy);
  jbyte val0 = val[0];
  jbyte val1 = val[1];
  jbyte val2 = val[2];
  val[0] = 0;
  val[1] = 1;
  val[2] = 2;
  printf("val0 = %d and val1 = %d and val2 = %d", val[0], val[1], val[2]);
  env->ReleaseByteArrayElements(arr, val, JNI_ABORT);
  jbyte *valu = env->GetByteArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %d and valu[1] = %d and valu[2] = %d", valu[0], valu[1], valu[2]);
  if((valu[0]==MAX_JBYTE) && (valu[1]==MIN_JBYTE) && (valu[2]==10)){
     return TestResult::PASS("ReleaseByteArrayElements(arr, val, JNI_ABORT) is correct");
  }else{
     return TestResult::FAIL("ReleaseByteArrayElements(arr, val, JNI_ABORT) is incorrect");
  }
}