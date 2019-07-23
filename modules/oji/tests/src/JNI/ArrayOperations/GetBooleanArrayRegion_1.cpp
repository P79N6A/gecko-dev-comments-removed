



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetBooleanArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jbooleanArray arr = env->NewBooleanArray(3);
  jsize start = 0;
  jsize leng = 3;
  jboolean buf[3];
  buf[0] = JNI_TRUE;
  buf[1] = JNI_FALSE;
  buf[2] = 0;
  env->SetBooleanArrayRegion(arr, start, leng, buf);

  jboolean val[3];
  env->GetBooleanArrayRegion(arr, start, leng, val);

  if((val[0]==JNI_TRUE)&&(val[1]==JNI_FALSE)&&(val[2]==0)){
     return TestResult::PASS("GetBooleanArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetBooleanArrayRegion(all right) returns incorrect value");
  }
}