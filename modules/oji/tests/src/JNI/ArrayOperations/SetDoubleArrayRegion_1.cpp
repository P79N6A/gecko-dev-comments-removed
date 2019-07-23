



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetDoubleArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(3);
  jsize start = 0;
  jsize leng = 3;
  jdouble buf[3];
  buf[0] = MAX_JDOUBLE;
  buf[1] = MIN_JDOUBLE;
  buf[2] = 0;
  env->SetDoubleArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jdouble *val = env->GetDoubleArrayElements(arr, NULL);
  jdouble val0 = val[0];
  jdouble val1 = val[1];
  jdouble val2 = val[2];

  if((val0==MAX_JDOUBLE)&&(val1==MIN_JDOUBLE)&&(val2==0)){
     return TestResult::PASS("SetDoubleArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("SetDoubleArrayRegion(all right) returns incorrect value");
  }

}
