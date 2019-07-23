



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewShortArray_2)
{
  GET_JNI_FOR_TEST

  jshortArray arr = env->NewShortArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewShortArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewShortArray(-5) returns incorrect value");
  }


}
