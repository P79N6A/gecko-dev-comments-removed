



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallIntMethod_14)
{
  GET_JNI_FOR_TEST

  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_thrown_excp", "(I)I");
  jvalue *args  = new jvalue[0];
  args[0].i = 10;
  jint value = env->CallIntMethodA(obj, MethodID, args);
  jthrowable excep = env->ExceptionOccurred();

  if((MethodID != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
     return TestResult::PASS("CallIntMethodA for public not inherited method that thrown exception really thrown exception");
  }else{
     return TestResult::FAIL("CallIntMethodA for public not inherited method that thrown exception do not really thrown exception");
  }

}

