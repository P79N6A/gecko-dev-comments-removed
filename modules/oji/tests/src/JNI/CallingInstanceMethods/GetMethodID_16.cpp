



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_16)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Test3");
  if(clazz == NULL){
      printf("Class is NULL!!!");
      return TestResult::FAIL("GetMethodID: class not found");
  }
  jmethodID methodID = env->GetMethodID(clazz, "Test_method", "()V");
  if(methodID != NULL){
     return TestResult::PASS("GetMethodID for public method from interface return correct value (0)");
  }else{
     return TestResult::FAIL("GetMethodID for public method from interface return incorrect value (non-0)");
  }

}
