



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_1)
{
  GET_JNI_FOR_TEST

  jmethodID MethodID = env->GetMethodID(NULL, "Print_string", "(Ljava/lang/String;)V");
  printf("ID of method = %d\n",  (int)MethodID); 


  if(MethodID == NULL){
     return TestResult::PASS("GetMethodID with class == NULL return 0, it's correct");
  }else{
     return TestResult::FAIL("GetMethodID with class == NULL doesn't return 0, it's incorrect");
  }

}
