



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field", "()I");
  
  jobject obj_new = env->NewObject(clazz, MethodID, 555);
  if(obj_new != NULL){
     return TestResult::FAIL("NewObject with legal class and illegal constructor ID");
  }else{
     return TestResult::PASS("NewObject with legal class and illegal constructor ID");
  }


  return TestResult::PASS("");

}
