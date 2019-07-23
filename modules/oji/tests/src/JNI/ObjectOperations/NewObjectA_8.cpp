



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectA_8)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test5", "<init>", "()V");

  jvalue args[1];
  jobject obj_new = env->NewObjectA(clazz, MethodID, args);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObjectA for class and package visible constructor");
  }else{
      return TestResult::FAIL("NewObjectA for class and package visible constructor");
  }
     

  return TestResult::PASS("");

}
