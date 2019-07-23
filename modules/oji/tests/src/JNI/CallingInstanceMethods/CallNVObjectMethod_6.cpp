



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualObjectMethod_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method5", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;");
  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jclass clazz_arr = env->FindClass("Ljava/lang/String;");
  jmethodID methodID_obj = env->GetMethodID(clazz_arr, "<init>", "(Ljava/lang/String;)V");
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj_arr = env->NewObject(clazz_arr, methodID_obj, str);
  jobjectArray arr = env->NewObjectArray(4, clazz_arr, obj_arr);
  jvalue *args  = new jvalue[10];
  args[0].z = JNI_FALSE;
  args[1].b = MIN_JBYTE;
  args[2].c = 'a';
  args[3].s = MAX_JSHORT;
  args[4].i = 123;
  args[5].j = 0;
  args[6].f = 0;
  args[7].d = 100;
  args[8].l = jpath;
  args[9].l = NULL;
  jobjectArray value = (jobjectArray)env->CallNonvirtualObjectMethodA(obj, env->GetSuperclass(clazz), MethodID, args);
  if(value == NULL){
     return TestResult::PASS("CallNonvirtualObjectMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return correct value");
  }else{
     return TestResult::FAIL("CallNonvirtualObjectMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)[Ljava/lang/String;) return incorrect value");
  }

}

