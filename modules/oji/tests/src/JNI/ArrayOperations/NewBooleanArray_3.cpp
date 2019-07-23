



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewBooleanArray_3)
{
  GET_JNI_FOR_TEST

  jbooleanArray arr = env->NewBooleanArray(0);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewBooleanArray(0) returns correct value");
  }else{
     return TestResult::FAIL("NewBooleanArray(0) returns incorrect value");
  }


}
