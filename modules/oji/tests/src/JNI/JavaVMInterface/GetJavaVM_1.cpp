



































#include "JNIEnvTests.h"
#include "JavaVMInterface.h"

JNI_OJIAPITest(JNIEnv_GetJavaVM_1)
{
  GET_JNI_FOR_TEST

  JavaVM **vm;
  jint value = env->GetJavaVM(NULL);
  if( (int)value < 0) {
     return TestResult::PASS("GetJavaVM work properly with NULL as JavaVM **");
  }else{
     return TestResult::FAIL("GetJavaVM does not work properly with NULL as JavaVM **");
  }


}
