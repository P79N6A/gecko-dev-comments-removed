



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetDoubleArrayElements_3)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(5);
  jsize start = 0;
  jsize leng = 3;
  jdouble buf[3];
  buf[0] = MAX_JDOUBLE;
  buf[1] = MIN_JDOUBLE;
  buf[2] = 0;
  env->SetDoubleArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jdouble *val = env->GetDoubleArrayElements(arr, &isCopy);
  jdouble val0 = val[0];
  jdouble val1 = val[1];
  jdouble val2 = val[2];

  if((val0==MAX_JDOUBLE)&&(val1==MIN_JDOUBLE)&&(val2==0)&&(isCopy!=JNI_FALSE)){
     return TestResult::PASS("GetDoubleArrayElements(arr, NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetDoubleArrayElements(arr, NULL) returns incorrect value");
  }

}
