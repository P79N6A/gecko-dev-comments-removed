



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectArray_2)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jobject obj = env->AllocObject(clazz);

  jarray arr = env->NewObjectArray(-5, clazz, NULL);

  jsize len = env->GetArrayLength(arr);
  if(arr==NULL){
     return TestResult::PASS("NewObjectArray(-5, clazz, NULL) returns correct value");
  }else{
     return TestResult::FAIL("NewObjectArray(-5, clazz, NULL) returns incorrect value");
  }


}
