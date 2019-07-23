



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_5)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test4");

  jobject obj_new = env->NewObject(clazz, (jmethodID)1000, 555);
  if(obj_new != NULL) {
      return TestResult::FAIL("NewObject for abstract class and invalid constructor ID");
  }else{
      return TestResult::PASS("NewObject for abstract class and invalid constructor ID");
  }
     

  return TestResult::PASS("");

}
