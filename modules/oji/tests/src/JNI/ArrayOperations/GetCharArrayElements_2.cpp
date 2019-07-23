



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetCharArrayElements_2)
{
  GET_JNI_FOR_TEST

  jcharArray arr = env->NewCharArray(5);
  jsize start = 0;
  jsize leng = 3;
  jchar buf[3];
  buf[0] = 'a';
  buf[1] = 'z';
  buf[2] = 0;
  env->SetCharArrayRegion(arr, start, leng, buf);

  jboolean isCopy = JNI_TRUE;
  jchar *val = env->GetCharArrayElements(arr, NULL);
  jchar val0 = val[0];
  jchar val1 = val[1];
  jchar val2 = val[2];

  if((val0=='a')&&(val1=='z')&&(val2==0)){
     return TestResult::PASS("GetCharArrayElements(arr, NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetCharArrayElements(arr, NULL) returns incorrect value");
  }

}
