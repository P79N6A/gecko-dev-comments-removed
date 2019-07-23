



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewByteArray_1)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(5);

  jsize len = env->GetArrayLength(arr);
  if((int)len==5){
     return TestResult::PASS("NewByteArray(5) returns correct value");
  }else{
     return TestResult::FAIL("NewByteArray(5) returns incorrect value");
  }


}
