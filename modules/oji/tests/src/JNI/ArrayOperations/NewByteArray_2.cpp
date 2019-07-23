



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewByteArray_2)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(-5);

  if(arr==NULL){
     return TestResult::PASS("NewByteArray(-5) returns correct value");
  }else{
     return TestResult::FAIL("NewByteArray(-5) returns incorrect value");
  }


}
