



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallIntMethod_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field", "()I");
  jfieldID fieldID = env->GetFieldID(clazz, "name_int", "I");
  env->SetIntField(obj, fieldID, 11);
  jint value = env->CallIntMethod(obj, MethodID, NULL);
  printf("The int field in clazz is : %d\n", (int)value);
  if((MethodID!=NULL) && (value == 11)){
      return TestResult::PASS("CallIntMethod with sig = \"()I\" return correct value");
  }else{
      return TestResult::FAIL("CallIntMethod with sig = \"()I\" return incorrect value");
  }

}
