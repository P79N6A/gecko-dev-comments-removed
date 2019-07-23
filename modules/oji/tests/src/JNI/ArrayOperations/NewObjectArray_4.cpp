



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectArray_4)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jobjectArray arr = env->NewObjectArray(4, clazz, obj);
  jsize len = env->GetArrayLength((jarray)arr);
  jstring str_n = (jstring)env->GetObjectArrayElement(arr, (jsize) 0);
  const char* chars = (char *) env->GetStringUTFChars(str_n, NULL);
  if(((int)len==4) && (strcmp(chars, "Test") == 0)){
     return TestResult::PASS("NewObjectArray(all correct with length > 0 ) returns correct value - empty array");
  }else{
     return TestResult::FAIL("NewObjectArray(all correct with length > 0 ) returns incorrect value");
  }


}
