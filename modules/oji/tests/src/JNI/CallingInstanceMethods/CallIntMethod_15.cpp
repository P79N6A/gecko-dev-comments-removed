



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallIntMethod_15)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Get_int_field", "()I");
  jfieldID fieldID = env->GetFieldID(clazz, "name_int", "I");
  env->SetIntField(obj, fieldID, 11);
  jvalue *args  = new jvalue[0];
  args[0].i = 10;
  jint value = env->CallIntMethodA(obj, MethodID, args);
  printf("The int field in clazz is : %d\n", (int)value);
  if((MethodID!=NULL) && (value == 11)){
      return TestResult::PASS("CallIntMethodA with sig = \"()I\" return correct value");
  }else{
      return TestResult::FAIL("CallIntMethodA with sig = \"()I\" return incorrect value");
  }

}
