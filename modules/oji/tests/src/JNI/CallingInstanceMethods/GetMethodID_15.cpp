



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_15)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetMethodID_METHOD("Test5", "Test_method", "()V");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  env->CallVoidMethod(obj, MethodID, NULL);
  return TestResult::PASS("GetMethodID for public method from children of interface return correct value");

}
