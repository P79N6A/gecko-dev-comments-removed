



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualIntMethod_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field", "()I");
  jfieldID fieldID = env->GetFieldID(clazz, "name_int", "I");
  env->SetIntField(obj, fieldID, 11);
  jint value = env->CallNonvirtualIntMethod(obj, env->GetSuperclass(clazz), MethodID, NULL);
  printf("The int field in clazz is : %d\n", (int)value);
  if((MethodID!=NULL) && (value == 11)){
      return TestResult::PASS("CallNonvirtualIntMethod with sig = \"()I\" return correct value");
  }else{
      return TestResult::FAIL("CallNonvirtualIntMethod with sig = \"()I\" return incorrect value");
  }

}
