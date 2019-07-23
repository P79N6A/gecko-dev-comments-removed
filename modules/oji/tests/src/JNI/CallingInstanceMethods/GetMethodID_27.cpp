



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_GetMethodID_27)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "<init>", "(I)V");
  jobject obj1 = env->NewObject(clazz, MethodID, 101);
  jfieldID fieldID = env->GetFieldID(clazz, "name_int", "I");
  printf("fieldID is : %d", fieldID);
  jint value = env->GetIntField(obj1, fieldID);
  printf("value is: %d", (int)value);
  if(value == 101){
     return TestResult::PASS("GetMethodID for public constructor in non-abstract class return correct value");
  }else{
     return TestResult::FAIL("GetMethodID for public constructor in non-abstract class return incorrect value");
  }
}
