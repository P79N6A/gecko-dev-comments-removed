



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"
#include "Test1.h"

JNI_OJIAPITest(JNIEnv_GetStaticMethodID_15)
{
  GET_JNI_FOR_TEST
  IMPLEMENT_GetStaticMethodID_METHOD("Test11", "mprint_static", "(I)V");
  if(clazz == NULL){
      printf("Class is NULL!!!");
  }
  env->CallStaticVoidMethod(clazz, MethodID, 100);
  if(MethodID != NULL){
     return TestResult::PASS("GetStaticMethodID for public native method from non-abstract class, not inherited return correct value");
  }else{
     return TestResult::FAIL("GetStaticMethodID for public native method from non-abstract class, not inherited return incorrect value");
  }
}

#if defined(__cplusplus)
extern "C" 
#endif
JNIEXPORT void JNICALL Java_Test11_mprint_1static
  (JNIEnv * env, jclass clazz, jint jInt){

  jclass clazz1 = env->FindClass("Test11");
  if(clazz1==NULL) {
      printf("There is no such class\n ");
  }
  jmethodID JPrintMethodID = env->GetStaticMethodID(clazz1, "jprint_static", "(I)V");
  printf("ID of get method = %d\n",(int)JPrintMethodID);
  env->CallStaticVoidMethod(clazz1, JPrintMethodID, (int)jInt);

    printf("OK!!!\n");
}
