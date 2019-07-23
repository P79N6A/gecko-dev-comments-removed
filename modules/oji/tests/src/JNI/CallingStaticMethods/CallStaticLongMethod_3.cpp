



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticLongMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method_long_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)J");
  jlong value = env->CallStaticLongMethod(clazz, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)MAX_JLONG, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MAX_JLONG){
     return TestResult::PASS("CallStaticLongMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)J) return correct value");
  }else{
     return TestResult::FAIL("CallStaticLongMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)J) return incorrect value");
  }

}

