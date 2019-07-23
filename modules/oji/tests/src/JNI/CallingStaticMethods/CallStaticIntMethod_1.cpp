



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticIntMethod_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Get_static_int_field", "()I");
  jfieldID fieldID = env->GetStaticFieldID(clazz, "static_name_int", "I");
  env->SetStaticIntField(clazz, fieldID, 11);
  jint value = env->CallStaticIntMethod(clazz, MethodID, NULL);
  printf("The static int field in clazz is : %d\n", (int)value);
  if((MethodID!=NULL) && (value == 11)){
      return TestResult::PASS("CallStaticIntMethod with sig = \"()I\" return correct value");
  }else{
      return TestResult::FAIL("CallStaticIntMethod with sig = \"()I\" return incorrect value");
  }

}
