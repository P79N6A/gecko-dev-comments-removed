



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewIntArray_2)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewIntArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewIntArray(-5) returns incorrect value");
  }


}
