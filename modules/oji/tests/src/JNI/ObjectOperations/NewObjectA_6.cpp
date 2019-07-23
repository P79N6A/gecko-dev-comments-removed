



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectA_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "()V");

  jvalue args[1];
  jobject obj_new = env->NewObjectA(clazz, MethodID, args);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObjectA for class and private constructor");
  }else{
      return TestResult::FAIL("NewObjectA for class and private constructor");
  }
     

  return TestResult::PASS("");

}
