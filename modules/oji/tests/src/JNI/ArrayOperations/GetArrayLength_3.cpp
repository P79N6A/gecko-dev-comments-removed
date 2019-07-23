



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetArrayLength_3)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jobject obj = env->AllocObject(clazz);

  jarray arr = env->NewObjectArray(0, clazz, NULL);

  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("GetArrayLength(empty) returns correct value");
  }else{
     return TestResult::FAIL("GetArrayLength(empty) returns incorrect value");
  }


}
