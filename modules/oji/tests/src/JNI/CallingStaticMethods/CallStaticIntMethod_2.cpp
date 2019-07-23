



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticIntMethod_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Get_static_int_field", "()I");
  jfieldID fieldID = env->GetStaticFieldID(clazz, "static_name_int", "I");
  env->SetStaticIntField(clazz, fieldID, 11);
  jvalue *args = new jvalue[1];
  args[0].i = 10;

  jint value = env->CallStaticIntMethodA(clazz, MethodID, args);
  printf("The static int field in clazz is : %d\n", (int)value);
  if((MethodID!=NULL) && (value == 11)){
      return TestResult::PASS("CallStaticIntMethodA with sig = \"()I\" return correct value");
  }else{
      return TestResult::FAIL("CallStaticIntMethodA with sig = \"()I\" return incorrect value");
  }

}
