



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetFloatArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jfloatArray arr = env->NewFloatArray(3);
  jsize start = 0;
  jsize leng = 3;
  jfloat buf[3];
  buf[0] = MAX_JFLOAT;
  buf[1] = MIN_JFLOAT;
  buf[2] = 0;
  env->SetFloatArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jfloat *val = env->GetFloatArrayElements(arr, NULL);
  jfloat val0 = val[0];
  jfloat val1 = val[1];
  jfloat val2 = val[2];

  if((val0==MAX_JFLOAT)&&(val1==MIN_JFLOAT)&&(val2==0)){
     return TestResult::PASS("SetFloatArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("SetFloatArrayRegion(all right) returns incorrect value");
  }

}
