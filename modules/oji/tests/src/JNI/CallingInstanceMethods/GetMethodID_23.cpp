



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_23)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Test4");
  jmethodID MethodID = env->GetMethodID(clazz, "Test9_abs_protected_int", "(I)I");
  printf("MethodID = %d", MethodID);
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  if(MethodID!=NULL){
    return TestResult::PASS("GetMethodID for protected abstract method from super-abstract class return correct value");
  }else{
    return TestResult::FAIL("GetMethodID for protected abstract method from super-abstract class return incorrect value");
  }
}
