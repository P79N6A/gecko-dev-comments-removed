



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_GetSuperclass_4)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Ljava/lang/Object;");

  jclass clazz1 = env->GetSuperclass(clazz);
  if(clazz1!=NULL){
      return TestResult::FAIL("GetSuperclass(lava.lang.Object) failed");
  }else{
      return TestResult::PASS("GetSuperclass(lava.lang.Object) return correct value");
  }


}
