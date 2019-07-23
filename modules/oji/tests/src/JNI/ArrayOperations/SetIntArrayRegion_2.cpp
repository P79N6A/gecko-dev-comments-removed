



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetIntArrayRegion_2)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(3);
  jsize start = 0;
  jsize leng = 3;
  jint buf[3];
  buf[0] = MAX_JINT;
  buf[1] = MIN_JINT;
  buf[2] = 0;
  env->SetIntArrayRegion(arr, start, leng, NULL);

  jboolean isCopy = JNI_TRUE;
  jint *val = env->GetIntArrayElements(arr, &isCopy);
  jint val0 = val[0];
  jint val1 = val[1];
  jint val2 = val[2];

  if((val0==0)&&(val1==0)&&(val2==0)){
     return TestResult::PASS("SetIntArrayRegion(with buf as NULL) returns correct value");
  }else{
     return TestResult::FAIL("SetIntArrayRegion(with buf as NULL) returns incorrect value");
  }
}