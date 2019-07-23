



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallByteMethod_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_byte", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)B");
  jbyte value = env->CallByteMethod(obj, MethodID, (jboolean)JNI_TRUE, (jbyte)MAX_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MAX_JBYTE){
     return TestResult::PASS("CallByteMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)B) return correct value");
  }else{
     return TestResult::FAIL("CallByteMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)B) return incorrect value");
  }

}

