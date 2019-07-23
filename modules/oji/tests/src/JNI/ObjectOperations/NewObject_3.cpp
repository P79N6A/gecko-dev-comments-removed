



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "(Ljava/lang/String;)V");
  jstring str = env->NewStringUTF("This is only a test string!!!!!");
  jobject obj_new = env->NewObject(clazz, MethodID, str);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObject with java\\lang\\String as argument");
  }else{
      return TestResult::FAIL("NewObject with java\\lang\\String as argument");
  }



  return TestResult::PASS("");

}
