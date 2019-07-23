



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"
#include "Test1.h"

JNI_OJIAPITest(JNIEnv_CallIntMethod_12)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test11", "Test1_method3_native", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I");
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
  jint value = env->CallIntMethodA(obj, MethodID, args);
  if(value == 121){
     return TestResult::PASS("CallIntMethodA for public synchronized not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return correct value");
  }else{
     return TestResult::FAIL("CallIntMethodA for public synchronized not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return incorrect value");
  }

}

#if defined(__cplusplus)
extern "C" 
#endif
JNIEXPORT jint JNICALL Java_Test11_Test1_1method3_1native
  (JNIEnv *, jobject, jboolean, jbyte, jchar, jshort, jint, jlong, jfloat, jdouble, jstring, jobjectArray){

  printf("Test1_method3_native passed!\n");
  return (jint)121;

}