



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetCharArrayElements_1)
{
  GET_JNI_FOR_TEST

  jcharArray arr = env->NewCharArray(5);

  jboolean isCopy = JNI_TRUE;
  jchar *val = env->GetCharArrayElements(NULL, &isCopy);
  printf("Function GetCharArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetCharArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetCharArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
