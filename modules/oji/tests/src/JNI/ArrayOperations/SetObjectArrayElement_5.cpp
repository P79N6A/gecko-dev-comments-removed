



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetObjectArrayElement_5)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jclass clazz_exp = env->FindClass("Ljava/lang/ArrayStoreException;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jobjectArray arr = env->NewObjectArray(4, clazz, obj);
  jchar str_chars1[] = {'T', 'e', 's', 't', '_', 'n', 'e', 'w'};
  jstring str1 = env->NewString(str_chars1, 8);
  jclass clazz_incor = env->FindClass("Test1");
  jobject obj_incor = env->AllocObject(clazz_incor);
  env->SetObjectArrayElement(arr, (jsize)2, obj_incor);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz_exp)){
    
     return TestResult::PASS("SetObjectArrayElement(incorrect object) returns correct value - empty array");
  }else{
     return TestResult::FAIL("SetObjectArrayElement(incorrect object) returns incorrect value");
  }

}
