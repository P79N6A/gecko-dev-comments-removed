



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_ReleaseCharArrayElements_2)
{
  GET_JNI_FOR_TEST

  jcharArray arr = env->NewCharArray(3);
  jsize start = 0;
  jsize leng = 3;
  jchar buf[3];
  buf[0] = 'a';
  buf[1] = 'z';
  buf[2] = '1';
  env->SetCharArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jchar *val = env->GetCharArrayElements(arr, &isCopy);
  jchar val0 = val[0];
  jchar val1 = val[1];
  jchar val2 = val[2];
  val[0] = 'q';
  val[1] = 'w';
  val[2] = 'e';
  printf("val0 = %c and val1 = %c and val2 = %c", val[0], val[1], val[2]);
  env->ReleaseCharArrayElements(arr, val, JNI_ABORT);
  jchar *valu = env->GetCharArrayElements(arr, &isCopy);
  printf("\n\nvalu[0] = %c and valu[1] = %c and valu[2] = %c", valu[0], valu[1], valu[2]);
  if((valu[0]=='a') && (valu[1]=='z') && (valu[2]=='1')){
     return TestResult::PASS("ReleaseCharArrayElements(arr, val, JNI_ABORT) is correct");
  }else{
     return TestResult::FAIL("ReleaseCharArrayElements(arr, val, JNI_ABORT) is incorrect");
  }
}