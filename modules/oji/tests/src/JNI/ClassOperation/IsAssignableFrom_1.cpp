



































#include "JNIEnvTests.h"
#include "ClassOperation.h"

JNI_OJIAPITest(JNIEnv_IsAssignableFrom_1)
{
  GET_JNI_FOR_TEST

  jclass clazz  = env->FindClass("Test1");
  jclass clazz1 = env->GetSuperclass(clazz);

  if(env->IsAssignableFrom(clazz1, clazz1) ){
      return TestResult::PASS("IsAssignableFrom(class, superclass) return correct value");
  }else{
      return TestResult::FAIL("IsAssignableFrom(class, superclass) return incorrect value");
  }


}
