



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewIntArray_3)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewIntArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewIntArray(0) returns incorrect value");
  }


}
