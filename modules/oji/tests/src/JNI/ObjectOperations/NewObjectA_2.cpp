



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectA_2)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");

  jvalue args[1];
  args[0].i = 555;
  jobject obj_new = env->NewObjectA(NULL, (jmethodID)1000, args);
  if(obj_new != NULL) {
      return TestResult::FAIL("NewObjectA for clazz = NULL and invalid constructor ID");
  }else{
      return TestResult::PASS("NewObjectA for clazz = NULL and invalid constructor ID");
  }
     

  return TestResult::PASS("");

}
