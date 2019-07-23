



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_39)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field_super", "()I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallIntMethod(obj, MethodID, NULL);
  if ((value == 0) && (MethodID!=NULL)){
      return TestResult::PASS("GetMethodID for public method from non-abstract class, inherited from superclass return correct value");
  }else{
      return TestResult::FAIL("GetMethodID for public method from non-abstract class, inherited from superclass return incorrect value");
  }
}
