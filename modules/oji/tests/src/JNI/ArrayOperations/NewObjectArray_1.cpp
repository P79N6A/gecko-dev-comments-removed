



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectArray_1)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jobject obj = env->AllocObject(clazz);

  jarray arr = env->NewObjectArray(5, clazz, NULL);

  jsize len = env->GetArrayLength(arr);
  if((int)len==5){
     return TestResult::PASS("NewObjectArray(obj ia NULL) returns correct value");
  }else{
     return TestResult::FAIL("NewObjectArray(obj ia NULL) returns incorrect value");
  }


}
