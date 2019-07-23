



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectA_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "(Ljava/lang/String;)V");

  jstring str = env->NewStringUTF("This is only a test string!!!!!");
  jvalue args[1];
  args[0].l = str;
  jobject obj_new = env->NewObjectA(clazz, MethodID, args);
  if(obj_new != NULL) {
      return TestResult::PASS("NewObjectA with java\\lang\\String as argument");
  }else{
      return TestResult::FAIL("NewObjectA with java\\lang\\String as argument");
  }



  return TestResult::PASS("");

}
