



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewLongArray_3)
{
  GET_JNI_FOR_TEST

  jlongArray arr = env->NewLongArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewLongArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewLongArray(0) returns incorrect value");
  }


}
