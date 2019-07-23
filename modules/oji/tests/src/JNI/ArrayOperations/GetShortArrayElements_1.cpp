



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetShortArrayElements_1)
{
  GET_JNI_FOR_TEST

  jshortArray arr = env->NewShortArray(5);

  jboolean isCopy = JNI_TRUE;
  jshort *val = env->GetShortArrayElements(NULL, &isCopy);
  printf("Function GetShortArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetShortArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetShortArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
