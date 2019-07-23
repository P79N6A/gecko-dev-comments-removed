



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticIntMethod_5)
{
  GET_JNI_FOR_TEST

  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_thrown_excp_static", "(I)I");
  jvalue *args  = new jvalue[0];
  args[0].i = 10;
  jint value = env->CallStaticIntMethodA(clazz, MethodID, args);
  jthrowable excep = env->ExceptionOccurred();

  if((MethodID != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
     return TestResult::PASS("CallStaticIntMethodA for public not inherited method that thrown exception really thrown exception");
  }else{
     return TestResult::FAIL("CallStaticIntMethodA for public not inherited method that thrown exception do not really thrown exception");
  }

}

