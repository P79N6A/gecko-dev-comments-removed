



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewByteArray_3)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewByteArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewByteArray(0) returns incorrect value");
  }


}
