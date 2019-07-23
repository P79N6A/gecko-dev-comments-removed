



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_GetObjectArrayElement_1)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jobjectArray arr = env->NewObjectArray(4, clazz, obj);
  jstring str_n = (jstring)env->GetObjectArrayElement(arr, (jsize) 0);
  const char* chars = (char *) env->GetStringUTFChars(str_n, NULL);
  if(strcmp(chars, "Test") == 0){
     return TestResult::PASS("GetObjectArrayElement(all correct index >= 0 ) returns correct value - empty array");
  }else{
     return TestResult::FAIL("GetObjectArrayElement(all correct index >= 0 ) returns incorrect value");
  }


}
