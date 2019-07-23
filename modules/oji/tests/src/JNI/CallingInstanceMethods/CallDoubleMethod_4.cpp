



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallDoubleMethod_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_double", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D");
  jdouble value = env->CallDoubleMethod(obj, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)10., (jdouble)MIN_JDOUBLE, (jobject)NULL, (jobject)NULL);
  if(value == MIN_JDOUBLE){
     return TestResult::PASS("CallDoubleMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D) return correct value");
  }else{
     return TestResult::FAIL("CallDoubleMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D) return incorrect value");
  }

}

