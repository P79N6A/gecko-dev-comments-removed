



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_30)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");
  jmethodID MethodID = env->GetMethodID(clazz, "<init>", "()V");
  printf("MethodID == %d\n", MethodID);
  if(MethodID != NULL){
     return TestResult::PASS("GetMethodID for protected constructor in abstract class return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for protected constructor in abstract class return incorrect value");
  }
}
