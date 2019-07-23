



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_19)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Get_int_field_super_static", "()I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallIntMethod(clazz, MethodID, NULL);
  if((value == 0) && (MethodID != NULL)){
      return TestResult::PASS("GetStaticMethodID for public method from non-abstract class, inherited from superclass return correct value");
  }else{
      return TestResult::FAIL("GetStaticMethodID for public method from non-abstract class, inherited from superclass return incorrect value");
  }
}
