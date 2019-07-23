



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewBooleanArray_2)
{
  GET_JNI_FOR_TEST

  jbooleanArray arr = env->NewBooleanArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewBooleanArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewBooleanArray(-5) returns incorrect value");
  }


}
