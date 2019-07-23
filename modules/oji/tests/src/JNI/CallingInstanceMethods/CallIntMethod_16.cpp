



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallIntMethod_16)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_int", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I");
  jint value = env->CallIntMethod(obj, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)MAX_JINT, (jlong)20, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MAX_JINT){
     return TestResult::PASS("CallIntMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return correct value");
  }else{
     return TestResult::FAIL("CallIntMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return incorrect value");
  }

}

