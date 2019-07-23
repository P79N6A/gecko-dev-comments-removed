



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewShortArray_3)
{
  GET_JNI_FOR_TEST

  jshortArray arr = env->NewShortArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewShortArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewShortArray(0) returns incorrect value");
  }


}
