



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");

  jobject obj_new = env->NewObject(NULL, (jmethodID)1000, 555);
  if(obj_new != NULL) {
      return TestResult::FAIL("NewObject for clazz = NULL and invalid constructor ID");
  }else{
      return TestResult::PASS("NewObject for clazz = NULL and invalid constructor ID");
  }
     

  return TestResult::PASS("");

}
