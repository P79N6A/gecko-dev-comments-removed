



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetBooleanArrayElements_1)
{
  GET_JNI_FOR_TEST

  jbooleanArray arr = env->NewBooleanArray(5);

  jboolean isCopy = JNI_TRUE;
  jboolean *val = env->GetBooleanArrayElements(NULL, &isCopy);
  printf("Function GetBooleanArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetBooleanArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetBooleanArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
