



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_SetObjectArrayElement_1)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jobjectArray arr = env->NewObjectArray(4, clazz, obj);
  jchar str_chars1[] = {'T', 'e', 's', 't', '_', 'n', 'e', 'w'};
  jstring str1 = env->NewString(str_chars1, 8);
  env->SetObjectArrayElement(arr, (jsize)0, str1);

  jstring str_n = (jstring)env->GetObjectArrayElement(arr, (jsize) 0);
  const char* chars = (char *) env->GetStringUTFChars(str_n, NULL);
  if(strcmp(chars, "Test_new") == 0){
     return TestResult::PASS("SetObjectArrayElement(all correct, index = 0 ) returns correct value - empty array");
  }else{
     return TestResult::FAIL("SetObjectArrayElement(all correct, index = 0 ) returns incorrect value");
  }

}
