



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_AllocObject_3)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test4");
  jobject obj = env->AllocObject(clazz);
  if(obj==NULL){
      printf("its allright!\n"); 
      return TestResult::PASS("AllocObject(abstract class) return correct value - NULL");
  }else{
      return TestResult::FAIL("AllocObject(abstract class) return incorrect value");
  }
}
