



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_28)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");
  jmethodID MethodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  printf("MethodID == %d\n", MethodID);
  if(MethodID != NULL){
     return TestResult::PASS("GetMethodID for public constructor in abstract class return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for public constructor in abstract class return incorrect value");
  }
}
