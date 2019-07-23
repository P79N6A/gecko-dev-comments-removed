



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectA_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "(I)V");
  
  jvalue args[1];
  args[0].i = 555;
  jobject obj_new = env->NewObjectA(clazz, MethodID, args);
  jfieldID fieldID = env->GetFieldID(clazz, "name_int", "I");
  printf("the name_int value is: %d\n\n", env->GetIntField(obj_new, fieldID));
  if(env->GetIntField(obj_new, fieldID) == 555){
      return TestResult::PASS("NewObjectA(all correct, int field) return correct value");
  }else{
      return TestResult::FAIL("NewObjectA(all correct, int field) return incorrect value");
  }
}
