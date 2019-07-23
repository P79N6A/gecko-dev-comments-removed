



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewDoubleArray_1)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(5);

  jsize len = env->GetArrayLength(arr);
  if((int)len==5){
     return TestResult::PASS("NewDoubleArray(5) returns correct value");
  }else{
     return TestResult::FAIL("NewDoubleArray(5) returns incorrect value");
  }


}
