



































#include "JNIEnvTests.h"
#include "GlobalAndLocalRefs.h"

JNI_OJIAPITest(JNIEnv_NewGlobalRef_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  jobject obj_ref = env->NewGlobalRef(obj);

  if(obj!=NULL && obj_ref!= NULL){
      return TestResult::PASS("NewGlobalRef return correct value with correct object as parameter");
  }else{
      return TestResult::FAIL("NewGlobalRef return NULL value with correct object as parameter");
  }

}
