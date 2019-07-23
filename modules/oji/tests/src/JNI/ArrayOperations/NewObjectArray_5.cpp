



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectArray_5)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jclass clazz_incor = env->FindClass("Test1");
  jmethodID methodID = env->GetMethodID(clazz_incor, "<init>", "(Ljava/lang/String;)V");
  jobject obj_incor = env->NewObject(clazz_incor, methodID, NULL);
  jobject obj = env->AllocObject(clazz);

  jarray arr = env->NewObjectArray(5, clazz, obj_incor);

  if(arr != NULL){ 
     return TestResult::PASS("NewObjectArray(obj is incorrect) returns correct value");
  }else{
     return TestResult::FAIL("NewObjectArray(obj is incorrect) returns incorrect value");
  }

}
