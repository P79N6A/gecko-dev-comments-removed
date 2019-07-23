



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_5)
{
  GET_JNI_FOR_TEST

        jclass clazz = env->FindClass("Test1");
        if(clazz == NULL){ 
            return TestResult::FAIL("Cannot find class"); 
        } 
        jmethodID MethodID = env->GetMethodID(clazz, "name_not_exist", "(Ljava/lang/String;)V");
        printf("ID of method = %d\n", (int)MethodID); 

  
  

  if(MethodID == NULL && env->ExceptionOccurred()) {
    env->ExceptionDescribe(); 
    return TestResult::PASS("GetMethodID for not existing name return 0, it's correct");
  }else{
    return TestResult::FAIL("GetMethodID for not existing name doesn't return 0, it's incorrect");
  }
}
