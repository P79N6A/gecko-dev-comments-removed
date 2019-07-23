



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"
#include "Test1.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualIntMethod_1)
{
  GET_JNI_FOR_TEST

  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_thrown_excp", "(I)I");
  jint value = env->CallNonvirtualIntMethod(obj, env->GetSuperclass(clazz), MethodID, 10);
  jthrowable excep = env->ExceptionOccurred();

  if((MethodID != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
     return TestResult::PASS("CallNonvirtualIntMethod for public not inherited method that thrown exception really thrown exception");
  }else{
     return TestResult::FAIL("CallNonvirtualIntMethod for public not inherited method that thrown exception do not really thrown exception");
  }

}

