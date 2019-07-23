



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetBooleanArrayElements_3)
{
  GET_JNI_FOR_TEST

  jbooleanArray arr = env->NewBooleanArray(5);
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

  if((val0==JNI_TRUE)&&(val1==JNI_FALSE)&&(val2==0)&&(isCopy!=JNI_FALSE)){
     return TestResult::PASS("GetBooleanArrayElements(arr, NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetBooleanArrayElements(arr, NULL) returns incorrect value");
  }

}
