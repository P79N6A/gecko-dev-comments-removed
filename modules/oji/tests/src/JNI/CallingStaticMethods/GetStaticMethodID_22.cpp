



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_22)
{
  GET_JNI_FOR_TEST
  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Get_int_field_super_private_static", "()I");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  jint value = env->CallStaticIntMethod(clazz, MethodID, NULL);

  jthrowable excep  = env->ExceptionOccurred();
  if(MethodID!=NULL){
     if((excep != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
       return TestResult::PASS("GetStaticMethodID for private method from non-abstract class, inherited from superclass return correct value");
     }else{
       if(SecENV){
         return TestResult::FAIL("GetStaticMethodID for private method from non-abstract class, inherited from superclass return incorrect value");
       }else{
         return TestResult::PASS("GetStaticMethodID for private method from non-abstract class, inherited from superclass return correct value");
       }
     }
  }else{
    return TestResult::FAIL("GetStaticMethodID for private method from non-abstract class, inherited from superclass return incorrect value");
  }
}
