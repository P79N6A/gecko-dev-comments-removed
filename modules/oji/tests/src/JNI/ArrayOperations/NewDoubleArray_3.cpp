



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewDoubleArray_3)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewDoubleArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewDoubleArray(0) returns incorrect value");
  }


}
