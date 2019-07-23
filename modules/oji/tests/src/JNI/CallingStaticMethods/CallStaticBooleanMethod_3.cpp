



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticBooleanMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method_bool_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Z");
  jboolean value = env->CallStaticBooleanMethod(clazz, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == JNI_TRUE){
     return TestResult::PASS("CallStaticBooleanMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Z) return correct value");
  }else{
     return TestResult::FAIL("CallStaticBooleanMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Z) return incorrect value");
  }

}

