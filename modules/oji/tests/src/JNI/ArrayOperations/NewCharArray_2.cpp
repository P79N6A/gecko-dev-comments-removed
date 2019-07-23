



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewCharArray_2)
{
  GET_JNI_FOR_TEST

  jcharArray arr = env->NewCharArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewCharArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewCharArray(-5) returns incorrect value");
  }


}
