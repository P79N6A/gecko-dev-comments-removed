



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_AllocObject_4)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test3");
  jobject obj = env->AllocObject(clazz);
  if(obj==NULL){
      printf("its allright!\n"); return TestResult::PASS("AllocObject(interface) return correct value - NULL");
  }else{
      return TestResult::FAIL("AllocObject(abstract class) return incorrect value");
  }
}
