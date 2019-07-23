



































#include "JNIEnvTests.h"
#include "JavaVMInterface.h"

JNI_OJIAPITest(JNIEnv_GetJavaVM_2)
{
  GET_JNI_FOR_TEST

  JavaVM *vm;
  jint value = env->GetJavaVM(&vm);
  if( (int)value == 0 && vm != NULL) {
     return TestResult::PASS("GetJavaVM work properly with correct parameters");
  }else{
     return TestResult::FAIL("GetJavaVM does not work properly with correct parameters");
  }


}
