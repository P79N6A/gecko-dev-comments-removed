



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualDoubleMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_double", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D");
  jdouble value = env->CallNonvirtualDoubleMethod(obj, env->GetSuperclass(clazz), MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)10., (jdouble)MAX_JDOUBLE, (jobject)NULL, (jobject)NULL);
  if(value == MAX_JDOUBLE){
     return TestResult::PASS("CallNonvirtualDoubleMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D) return correct value");
  }else{
     return TestResult::FAIL("CallNonvirtualDoubleMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D) return incorrect value");
  }

}

