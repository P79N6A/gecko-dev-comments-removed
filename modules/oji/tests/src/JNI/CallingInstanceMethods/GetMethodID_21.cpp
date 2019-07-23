



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_21)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Test4");
  jmethodID MethodID = env->GetMethodID(clazz, "Abs_nomod_abstract_int", "(I)I");
  printf("MethodID = %d", MethodID);
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  if(MethodID!=NULL){
    return TestResult::PASS("GetMethodID for no-modifier abstract method from abstract class return correct value");
  }else{
    return TestResult::FAIL("GetMethodID for no-modifier abstract method from abstract class return incorrect value");
  }
}
