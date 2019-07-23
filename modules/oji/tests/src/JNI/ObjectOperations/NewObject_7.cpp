



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_7)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test2", "<init>", "()V");

  jobject obj_new = env->NewObject(clazz, MethodID, NULL);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObject for class and protected constructor");
  }else{
      return TestResult::FAIL("NewObject for class and protected constructor");
  }
     

  return TestResult::PASS("");

}
