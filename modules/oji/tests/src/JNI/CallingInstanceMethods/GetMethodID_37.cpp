



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_37)
{
  GET_JNI_FOR_TEST
  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetMethodID_METHOD("Test1", "Print_string_super_nomod", "(Ljava/lang/String;)V");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  env->CallVoidMethod(obj, MethodID, NULL);

  jthrowable excep  = env->ExceptionOccurred();

  if(MethodID!=NULL){
     if((excep != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
       return TestResult::PASS("GetMethodID for no-modifiers method from non-abstract class, inherited from superclass return correct value");
     }else{
       if(SecENV){
         return TestResult::FAIL("GetMethodID for no-modifiers method from non-abstract class, inherited from superclass return incorrect value");
       }else{
         return TestResult::PASS("GetMethodID for no-modifiers method from non-abstract class, inherited from superclass return correct value");
       }
     }
  }else{
    return TestResult::FAIL("GetMethodID for no-modifiers method from non-abstract class, inherited from superclass return incorrect value");
  }
}
