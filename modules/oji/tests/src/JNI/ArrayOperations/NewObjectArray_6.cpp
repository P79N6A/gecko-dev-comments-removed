



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectArray_6)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jobject obj = env->AllocObject(clazz);

  jarray arr = env->NewObjectArray(5, NULL, obj);

  if(arr==NULL){
     return TestResult::PASS("NewObjectArray(obj is incorrect) returns correct value");
  }else{
     return TestResult::FAIL("NewObjectArray(obj is incorrect) returns incorrect value");
  }

}
