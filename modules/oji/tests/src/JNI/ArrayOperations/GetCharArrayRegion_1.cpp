



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetCharArrayRegion_1)
{
  GET_JNI_FOR_TEST

  jcharArray arr = env->NewCharArray(3);
  jsize start = 0;
  jsize leng = 3;
  jchar buf[3];
  buf[0] = 'a';
  buf[1] = 'z';
  buf[2] = '0';
  env->SetCharArrayRegion(arr, start, leng, buf);

  jchar val[3];
  env->GetCharArrayRegion(arr, start, leng, val);

  if((val[0]=='a')&&(val[1]=='z')&&(val[2]=='0')){
     return TestResult::PASS("GetCharArrayRegion(all right) returns correct value");
  }else{
     return TestResult::FAIL("GetCharArrayRegion(all right) returns incorrect value");
  }
}