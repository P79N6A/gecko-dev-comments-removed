



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetObjectArrayElement_5)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jclass clazz_exp = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jobjectArray arr = env->NewObjectArray(4, clazz, obj);
  jstring str_n = (jstring)env->GetObjectArrayElement(arr, (jsize) 6);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz_exp)){
    printf("ArrayIndexOutOfBoundsException is thrown. It is correct!\n");
     return TestResult::PASS("GetObjectArrayElement(index > length ) returns correct value - empty array");
  }else{
     return TestResult::FAIL("GetObjectArrayElement(index > length ) returns incorrect value");
  }

}
