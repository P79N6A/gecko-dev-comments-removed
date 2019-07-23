



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewLongArray_2)
{
  GET_JNI_FOR_TEST

  jlongArray arr = env->NewLongArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewLongArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewLongArray(-5) returns incorrect value");
  }


}
