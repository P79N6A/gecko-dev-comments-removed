



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewFloatArray_1)
{
  GET_JNI_FOR_TEST

  jfloatArray arr = env->NewFloatArray(5);

  jsize len = env->GetArrayLength(arr);
  if((int)len==5){
     return TestResult::PASS("NewFloatArray(5) returns correct value");
  }else{
     return TestResult::FAIL("NewFloatArray(5) returns incorrect value");
  }


}
