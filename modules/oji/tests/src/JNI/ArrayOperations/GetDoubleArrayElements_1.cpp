



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetDoubleArrayElements_1)
{
  GET_JNI_FOR_TEST

  jdoubleArray arr = env->NewDoubleArray(5);

  jboolean isCopy = JNI_TRUE;
  jdouble *val = env->GetDoubleArrayElements(NULL, &isCopy);
  printf("Function GetDoubleArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetDoubleArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetDoubleArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
