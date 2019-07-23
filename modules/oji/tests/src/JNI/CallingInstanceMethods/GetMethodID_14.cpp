



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"
#include "Test1.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_14)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetMethodID_METHOD("Test11", "mprint", "(I)V");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  env->CallVoidMethod(obj, MethodID, 100);
  if(MethodID != NULL){
     return TestResult::PASS("GetMethodID for public native method from non-abstract class, not inherited return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for public native method from non-abstract class, not inherited return incorrect value");
  }
}


#if defined(__cplusplus)
extern "C" 
#endif
JNIEXPORT void JNICALL Java_Test11_mprint
  (JNIEnv * env, jobject obj, jint jInt){

  jclass clazz = env->FindClass("Test11");
  if(clazz==NULL) {
      printf("There is no such class\n ");
  }
  jmethodID JPrintMethodID = env->GetMethodID(clazz, "jprint", "(I)V");
  printf("ID of get method = %d\n",(int)JPrintMethodID);
  env->CallVoidMethod(obj, JPrintMethodID, (int)jInt);

    printf("OK!!!\n");
}
