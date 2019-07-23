



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_13)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_public_final_static", "(I)I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallStaticIntMethod(clazz, MethodID, 100);
  printf("value == %d", value);
  if(value == 100){
     return TestResult::PASS("GetStaticMethodID for public static final method from non-abstract class, not inherited return correct value");
  }else{
     return TestResult::FAIL("GetStaticMethodID for public static final method from non-abstract class, not inherited return incorrect value");
  }
}
