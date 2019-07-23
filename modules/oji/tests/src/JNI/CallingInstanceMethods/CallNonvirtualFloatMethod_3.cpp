



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualFloatMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_float", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F");
  jfloat value = env->CallNonvirtualFloatMethod(obj, env->GetSuperclass(clazz), MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)MIN_JFLOAT, (jdouble)100, (jobject)NULL, (jobject)NULL);
  if(value == MIN_JFLOAT){
     return TestResult::PASS("CallNonvirtualFloatMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F) return correct value");
  }else{
     return TestResult::FAIL("CallNonvirtualFloatMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F) return incorrect value");
  }

}

