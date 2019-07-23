



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_13)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_public_sinc", "(I)I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallIntMethod(obj, MethodID, 100);
  printf("value == %d", value);
  if(value == 100){
     return TestResult::PASS("GetMethodID for public synchronized method from non-abstract class, not inherited return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for public synchronized method from non-abstract class, not inherited return incorrect value");
  }
}
