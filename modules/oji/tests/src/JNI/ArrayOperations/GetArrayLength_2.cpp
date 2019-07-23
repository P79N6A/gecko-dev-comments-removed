



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetArrayLength_2)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jobject obj = env->AllocObject(clazz);


  jsize len = env->GetArrayLength(NULL);
  if((int)len==0){
     return TestResult::PASS("GetArrayLength(NULL) returns correct value");
  }else{
     return TestResult::FAIL("GetArrayLength(NULL) returns incorrect value");
  }


}
