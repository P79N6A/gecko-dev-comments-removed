



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_11)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field", "()I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallIntMethod(obj, MethodID, NULL);
  if (value == 0){
      return TestResult::PASS("GetMethodID for public method from non-abstract class, not inherited return correct value");
  }else{
      return TestResult::FAIL("GetMethodID for public method from non-abstract class, not inherited return incorrect value");
  }
}
