



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewFloatArray_2)
{
  GET_JNI_FOR_TEST

  jfloatArray arr = env->NewFloatArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewFloatArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewFloatArray(-5) returns incorrect value");
  }


}
