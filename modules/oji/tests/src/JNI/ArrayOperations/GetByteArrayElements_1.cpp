



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetByteArrayElements_1)
{
  GET_JNI_FOR_TEST

  jbyteArray arr = env->NewByteArray(5);

  jboolean isCopy = JNI_TRUE;
  jbyte *val = env->GetByteArrayElements(NULL, &isCopy);
  printf("Function GetByteArrayElements(NULL, isCopy) is not crushed!\n\n");
  if(val==NULL){
     return TestResult::PASS("GetByteArrayElements(NULL, NOT_NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetByteArrayElements(NULL, NOT_NULL) returns incorrect value");
  }

}
