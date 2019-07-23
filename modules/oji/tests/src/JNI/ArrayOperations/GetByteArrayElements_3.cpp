



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetByteArrayElements_3)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(5);
  jsize start = 0;
  jsize leng = 3;
  jbyte buf[3];
  buf[0] = MAX_JBYTE;
  buf[1] = MIN_JBYTE;
  buf[2] = 0;
  env->SetByteArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jbyte *val = env->GetByteArrayElements(arr, &isCopy);
  jbyte val0 = val[0];
  jbyte val1 = val[1];
  jbyte val2 = val[2];

  if((val0==MAX_JBYTE)&&(val1==MIN_JBYTE)&&(val2==0)&&(isCopy!=JNI_FALSE)){
     return TestResult::PASS("GetByteArrayElements(arr, NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetByteArrayElements(arr, NULL) returns incorrect value");
  }

}
