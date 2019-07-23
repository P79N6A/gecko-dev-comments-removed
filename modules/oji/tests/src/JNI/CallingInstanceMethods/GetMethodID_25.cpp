



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_25)
{
  GET_JNI_FOR_TEST

  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  if(clazz_exp == NULL){ printf("clazz_exp is NULL\n");}
  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "(S)V");

  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jobject obj1 = env->NewObject(clazz, MethodID, 10);
  jthrowable excep = env->ExceptionOccurred();
  if(MethodID!=NULL){
     if((excep != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
       printf("Exception Occurred, it is correct!!!!\n");
       return TestResult::PASS("GetMethodID for no-modifiers constructor in non-abstract class return correct value");
     }else{
       if(SecENV){
         return TestResult::FAIL("GetMethodID for no-modifiers constructor in non-abstract class return incorrect value");
       }else{
         return TestResult::PASS("GetMethodID for no-modifiers constructor in non-abstract class return correct value");
       }
     }
  }else{
    return TestResult::FAIL("GetMethodID for no-modifiers constructor in non-abstract class return incorrect value");
  }
}
