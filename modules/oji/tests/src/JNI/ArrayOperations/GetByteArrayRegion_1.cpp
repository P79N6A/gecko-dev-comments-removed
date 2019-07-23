



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetByteArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(3);
  jsize start = 0;
  jsize leng = 3;
  jbyte buf[3];
  buf[0] = MAX_JBYTE;
  buf[1] = MIN_JBYTE;
  buf[2] = 0;
  env->SetByteArrayRegion(arr, start, leng, buf);

  jbyte val[3];
  env->GetByteArrayRegion(arr, start, leng, val);

  if((val[0]==MAX_JBYTE)&&(val[1]==MIN_JBYTE)&&(val[2]==0)){
     return TestResult::PASS("GetByteArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetByteArrayRegion(all right) returns incorrect value");
  }
}