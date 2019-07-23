



































#include "JNIEnvTests.h"
#include "GlobalAndLocalRefs.h"

JNI_OJIAPITest(JNIEnv_NewGlobalRef_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1");
  jobject obj = env->AllocObject(clazz);
  jobject obj_ref = env->NewGlobalRef(NULL);

  if(obj!=NULL && obj_ref== NULL){
      return TestResult::PASS("NewGlobalRef return correct value with object=NULL");
  }else{
      return TestResult::FAIL("NewGlobalRef do not return NULL value with object=NULL");
  }

}
