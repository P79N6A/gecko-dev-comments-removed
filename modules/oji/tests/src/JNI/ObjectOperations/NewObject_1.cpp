



































#include "JNIEnvTests.h"
#include "ObjectOperations.h"

JNI_OJIAPITest(JNIEnv_NewObject_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "(I)V");
  
  jobject obj_new = env->NewObject(clazz, MethodID, 555);
  jfieldID fieldID = env->GetFieldID(clazz, "name_int", "I");
  printf("the name_int value is: %d\n\n", env->GetIntField(obj_new, fieldID));
  if(env->GetIntField(obj_new, fieldID) == 555){
      return TestResult::PASS("NewObject(all correct, int field) return correct value");
  }else{
      return TestResult::FAIL("NewObject(all correct, int field) return incorrect value");
  }

}
