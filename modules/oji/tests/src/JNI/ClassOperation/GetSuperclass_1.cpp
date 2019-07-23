



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_GetSuperclass_1)
{
  GET_JNI_FOR_TEST

  jclass clazz1 = env->GetSuperclass(NULL);
  printf("GetSuperPassed");

  if(clazz1 == NULL ){
    return TestResult::PASS("GetSuperclass(NULL) return NULL - correct");
  }else{
    return TestResult::FAIL("GetSuperclass(NULL) return not NULL - incorrect");
  }
}
