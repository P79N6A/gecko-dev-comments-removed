



































#include "JNIEnvTests.h"

JNI_OJIAPITest(JNIEnv_GetVersion_1)
{
  GET_JNI_FOR_TEST

  jint vers = env->GetVersion();
  printf("The version is: %d\n\n", (int)vers);

  return TestResult::PASS("GetVersion passed");

}
