



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetShortArrayElements_3)
{
  GET_JNI_FOR_TEST

  jshortArray arr = env->NewShortArray(5);
  jsize start = 0;
  jsize leng = 3;
  jshort buf[3];
  buf[0] = MAX_JSHORT;
  buf[1] = MIN_JSHORT;
  buf[2] = 0;
  env->SetShortArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jshort *val = env->GetShortArrayElements(arr, &isCopy);
  jshort val0 = val[0];
  jshort val1 = val[1];
  jshort val2 = val[2];

  if((val0==MAX_JSHORT)&&(val1==MIN_JSHORT)&&(val2==0)&&(isCopy!=JNI_FALSE)){
     return TestResult::PASS("GetShortArrayElements(arr, NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetShortArrayElements(arr, NULL) returns incorrect value");
  }

}
