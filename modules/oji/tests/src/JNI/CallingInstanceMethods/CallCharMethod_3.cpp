



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallCharMethod_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_char", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)C");
  jchar value = env->CallCharMethod(obj, MethodID, JNI_TRUE, MIN_JBYTE, 'a', 1, 123, 0, 0, 100, NULL, NULL);
  if(value == 'a'){
     return TestResult::PASS("CallCharMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)C) return correct value");
  }else{
     return TestResult::FAIL("CallCharMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)C) return incorrect value");
  }

}

