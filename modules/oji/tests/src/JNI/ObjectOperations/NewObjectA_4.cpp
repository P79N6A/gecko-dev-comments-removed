



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectA_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field", "()I");
  
  jvalue args[1];
  args[0].i = 555;
  jobject obj_new = env->NewObjectA(clazz, MethodID, args);
  if(obj_new != NULL){
     return TestResult::FAIL("NewObjectA with legal class and illegal constructor ID");
  }else{
     return TestResult::PASS("NewObjectA with legal class and illegal constructor ID");
  }


  return TestResult::PASS("");

}
