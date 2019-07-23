



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_10)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method5", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;");
  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jvalue *args  = new jvalue[10];
  args[0].z = JNI_TRUE;
  args[1].b = 0;
  args[2].c = 'a';
  args[3].s = 1;
  args[4].i = 123;
  args[5].j = 0;
  args[6].f = 0;
  args[7].d = 100;
  args[8].l = jpath;
  args[9].l = NULL;
  jobject value = env->CallObjectMethodA(obj, MethodID, args);
  if(value == NULL){
     return TestResult::PASS("GetMethodID for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return incorrect value");
  }
}
