



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewCharArray_3)
{
  GET_JNI_FOR_TEST

  jcharArray arr = env->NewCharArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewCharArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewCharArray(0) returns incorrect value");
  }


}
