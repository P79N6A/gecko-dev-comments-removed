



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_18)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Test10");
  jmethodID MethodID = env->GetMethodID(clazz, "Test8_method1", "()V");
  printf("MethodID = %d", MethodID);
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  if(MethodID != NULL){
    return TestResult::PASS("GetMethodID for public method from abstract class inherited from interface return correct value");
  }else{
    return TestResult::FAIL("GetMethodID for public method from abstract class inherited from interface return incorrect value");
  }
}
