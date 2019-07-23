



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewIntArray_1)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(5);

  jsize len = env->GetArrayLength(arr);
  if((int)len==5){
     return TestResult::PASS("NewIntArray(5) returns correct value");
  }else{
     return TestResult::FAIL("NewIntArray(5) returns incorrect value");
  }


}
