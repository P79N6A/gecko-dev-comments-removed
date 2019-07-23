



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticFloatMethod_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method_float_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F");
  jfloat value = env->CallStaticFloatMethod(clazz, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)MAX_JFLOAT, (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MAX_JFLOAT){
     return TestResult::PASS("CallStaticFloatMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F) return correct value");
  }else{
     return TestResult::FAIL("CallStaticFloatMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F) return incorrect value");
  }

}

