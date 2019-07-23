



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_12)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Get_static_int_field", "()I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallStaticIntMethod(clazz, MethodID, NULL);
  if ((value == 1) && (MethodID != NULL)){
      return TestResult::PASS("GetStaticMethodID for public static method from non-abstract class, not inherited return correct value");
  }else{
      return TestResult::FAIL("GetStaticMethodID for public static method from non-abstract class, not inherited return incorrect value");
  }
}
