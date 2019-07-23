



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewFloatArray_3)
{
  GET_JNI_FOR_TEST

  jfloatArray arr = env->NewFloatArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewFloatArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewFloatArray(0) returns incorrect value");
  }


}
