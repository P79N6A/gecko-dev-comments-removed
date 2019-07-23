



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_29)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");
  jmethodID MethodID = env->GetMethodID(clazz, "<init>", "(I)V");
  printf("MethodID == %d\n", MethodID);
  if(MethodID != NULL){
     return TestResult::PASS("GetMethodID for private constructor in abstract class return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for private constructor in abstract class return incorrect value");
  }
}
