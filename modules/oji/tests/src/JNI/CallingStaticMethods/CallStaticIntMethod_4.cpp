



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticIntMethod_4)
{
  GET_JNI_FOR_TEST

  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_thrown_excp_static", "(I)I");
  jint value = env->CallStaticIntMethod(clazz, MethodID, 10);
  jthrowable excep = env->ExceptionOccurred();

  if((MethodID != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
     return TestResult::PASS("CallStaticIntMethod for public not inherited method that thrown exception really thrown exception");
  }else{
     return TestResult::FAIL("CallStaticIntMethod for public not inherited method that thrown exception do not really thrown exception");
  }

}

