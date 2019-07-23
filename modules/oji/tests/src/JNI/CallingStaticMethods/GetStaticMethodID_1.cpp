



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_1)
{
  GET_JNI_FOR_TEST

  jmethodID MethodID = env->GetStaticMethodID(NULL, "Print_string_static", "(Ljava/lang/String;)V");
  printf("ID of method = %d\n",  (int)MethodID); 


  if(MethodID == NULL){
     return TestResult::PASS("GetStaticMethodID with class == NULL return 0, it's correct");
  }else{
     return TestResult::FAIL("GetStaticMethodID with class == NULL doesn't return 0, it's incorrect");
  }

}
