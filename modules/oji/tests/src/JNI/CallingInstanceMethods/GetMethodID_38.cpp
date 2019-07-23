



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_38)
{
  GET_JNI_FOR_TEST
  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field_super_private", "()I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallIntMethod(obj, MethodID, NULL);
  printf("value is: %d", value);

  jthrowable excep  = env->ExceptionOccurred();

  if(MethodID!=NULL){
     if((excep != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
       return TestResult::PASS("GetMethodID for private method from non-abstract class, inherited from superclass return correct value");
     }else{
       if(SecENV){
         return TestResult::FAIL("GetMethodID for private method from non-abstract class, inherited from superclass return incorrect value");
       }else{
         return TestResult::PASS("GetMethodID for private method from non-abstract class, inherited from superclass return correct value");
       }
     }
  }else{
    return TestResult::FAIL("GetMethodID for private method from non-abstract class, inherited from superclass return incorrect value");
  }

}
