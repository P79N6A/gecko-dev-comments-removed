



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetObjectArrayElement_3)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jclass clazz_exp = env->FindClass("Ljava/lang/ArrayIndexOutOfBoundsException;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jobjectArray arr = env->NewObjectArray(4, clazz, obj);
  jchar str_chars1[] = {'T', 'e', 's', 't', '_', 'n', 'e', 'w'};
  jstring str1 = env->NewString(str_chars1, 8);
  env->SetObjectArrayElement(arr, (jsize)-1, str1);
  jthrowable excep  = env->ExceptionOccurred();
  if(env->IsInstanceOf(excep, clazz_exp)){
    printf("ArrayIndexOutOfBoundsException is thrown. It is correct!\n");
     return TestResult::PASS("SetObjectArrayElement(index < 0 ) returns correct value - empty array");
  }else{
     return TestResult::FAIL("SetObjectArrayElement(index < 0 ) returns incorrect value");
  }

}
