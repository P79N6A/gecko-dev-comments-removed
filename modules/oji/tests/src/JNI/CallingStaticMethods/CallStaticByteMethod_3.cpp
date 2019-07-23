



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticByteMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method_byte_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)B");
  jbyte value = env->CallStaticByteMethod(clazz, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MIN_JBYTE){
     return TestResult::PASS("CallStaticByteMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)B) return correct value");
  }else{
     return TestResult::FAIL("CallStaticByteMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)B) return incorrect value");
  }

}

