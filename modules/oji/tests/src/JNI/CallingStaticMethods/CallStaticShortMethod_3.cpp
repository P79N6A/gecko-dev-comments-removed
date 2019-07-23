



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticShortMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method_short_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)S");
  jshort value = env->CallStaticShortMethod(clazz, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)MAX_JSHORT, (jint)123, (jlong)20, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MAX_JSHORT){
     return TestResult::PASS("CallStaticShortMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)S) return correct value");
  }else{
     return TestResult::FAIL("CallStaticShortMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)S) return incorrect value");
  }

}

