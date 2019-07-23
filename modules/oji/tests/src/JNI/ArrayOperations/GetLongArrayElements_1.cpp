



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetLongArrayElements_1)
{
  GET_JNI_FOR_TEST

  jlongArray arr = env->NewLongArray(5);

  jboolean isCopy = JNI_TRUE;
  jlong *val = env->GetLongArrayElements(NULL, &isCopy);
  printf("Function GetLongArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetLongArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetLongArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
