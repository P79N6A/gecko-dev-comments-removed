



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetIntArrayElements_1)
{
  GET_JNI_FOR_TEST

  jintArray arr = env->NewIntArray(5);

  jboolean isCopy = JNI_TRUE;
  jint *val = env->GetIntArrayElements(NULL, &isCopy);
  printf("Function GetIntArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetIntArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetIntArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
