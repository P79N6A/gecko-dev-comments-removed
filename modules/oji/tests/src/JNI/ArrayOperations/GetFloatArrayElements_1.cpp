



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetFloatArrayElements_1)
{
  GET_JNI_FOR_TEST

  jfloatArray arr = env->NewFloatArray(5);

  jboolean isCopy = JNI_TRUE;
  jfloat *val = env->GetFloatArrayElements(NULL, &isCopy);
  printf("Function GetFloatArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetFloatArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetFloatArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
