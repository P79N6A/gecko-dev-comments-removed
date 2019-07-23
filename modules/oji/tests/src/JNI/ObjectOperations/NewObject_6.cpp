



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "()V");

  jobject obj_new = env->NewObject(clazz, MethodID, NULL);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObject for class and private constructor");
  }else{
      return TestResult::FAIL("NewObject for class and private constructor");
  }
     

  return TestResult::PASS("");

}
