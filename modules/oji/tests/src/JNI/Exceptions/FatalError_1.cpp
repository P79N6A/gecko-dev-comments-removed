



































#include "JNIEnvTests.h"
#include "Exceptions.h"

JNI_OJIAPITest(JNIEnv_FatalError_1)
{
  GET_JNI_FOR_TEST

  env->FatalError("aaaaaa");
  return TestResult::PASS("Crush - correct");
}
