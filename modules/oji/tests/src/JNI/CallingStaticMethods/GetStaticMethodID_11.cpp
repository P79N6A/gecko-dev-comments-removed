



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_11)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test2_override_static", "(I)I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallStaticIntMethod(clazz, MethodID, 100);
  printf("value == %d", value);
  if(value == 101){
     return TestResult::PASS("GetStaticMethodID for public overrided method from non-abstract class, not inherited return correct value");
  }else{
     return TestResult::FAIL("GetStaticMethodID for public overrided method from non-abstract class, not inherited return incorrect value");
  }
}
