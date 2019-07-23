



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetLongArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jlongArray arr = env->NewLongArray(3);
  jsize start = 0;
  jsize leng = 3;
  jlong buf[3];
  buf[0] = MAX_JLONG;
  buf[1] = MIN_JLONG;
  buf[2] = 0;
  env->SetLongArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jlong *val = env->GetLongArrayElements(arr, NULL);
  jlong val0 = val[0];
  jlong val1 = val[1];
  jlong val2 = val[2];

  if((val0==MAX_JLONG)&&(val1==MIN_JLONG)&&(val2==0)){
     return TestResult::PASS("SetLongArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("SetLongArrayRegion(all right) returns incorrect value");
  }

}
