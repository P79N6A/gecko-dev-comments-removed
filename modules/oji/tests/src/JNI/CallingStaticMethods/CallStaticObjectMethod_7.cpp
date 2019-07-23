



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticObjectMethod_7)
{
  GET_JNI_FOR_TEST

  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jclass clazz_arr = env->FindClass("Ljava/lang/String;");
  jmethodID methodID_obj = env->GetMethodID(clazz_arr, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj_arr = env->NewObject(clazz_arr, methodID_obj, str);
  jobjectArray arr = env->NewObjectArray(4, clazz_arr, obj_arr);
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method5_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;");
  char *path = "asdf";
  if(arr==NULL){
      printf("arr is NULL!\n");
  }
  jobjectArray value = (jobjectArray)env->CallStaticObjectMethod(clazz, MethodID, (jboolean)JNI_TRUE, (jbyte)MIN_JBYTE, (jchar)0, (jshort)1, (jint)123, (jlong)20, (jfloat)10., (jdouble)100, (jobject)NULL, (jobject)arr);
  if(value == NULL){
     printf("value is NULL!!! \n\n");
     return TestResult::FAIL("CallObjectMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return NULL");
  }
  jstring str_ret = (jstring)env->GetObjectArrayElement(value, (jsize) 1);
  char* str_chars_ret = (char *) env->GetStringUTFChars(str, NULL);
  jsize len_ret = env->GetArrayLength(value);
  if((len_ret == 4) && (strcmp(str_chars_ret, "Test") == 0)){
     return TestResult::PASS("CallObjectMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return correct value");
  }else{
     return TestResult::FAIL("CallObjectMethod for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return incorrect value");
  }

}

