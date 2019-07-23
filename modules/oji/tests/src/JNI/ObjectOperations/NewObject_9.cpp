



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_9)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test6", "<init>", "()V");

  jobject obj_new = env->NewObject(clazz, MethodID, NULL);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObject for class and public constructor without arguments");
  }else{
      return TestResult::FAIL("NewObject for class and public constructor without arguments");
  }
     

  return TestResult::PASS("");

}
