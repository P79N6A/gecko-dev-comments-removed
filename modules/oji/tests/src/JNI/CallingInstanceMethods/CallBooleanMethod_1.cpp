



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallBooleanMethod_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_bool", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Z");
  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jvalue *args  = new jvalue[10];
  args[0].z = JNI_TRUE;
  args[1].b = 0;
  args[2].c = 'a';
  args[3].s = 1;
  args[4].i = 123;
  args[5].j = 0;
  args[6].f = 0;
  args[7].d = 100;
  args[8].l = jpath;
  args[9].l = NULL;
  jboolean value = env->CallBooleanMethodA(obj, MethodID, args);
  if(value == JNI_TRUE){
     return TestResult::PASS("CallBooleanMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Z) return correct value");
  }else{
     return TestResult::FAIL("CallBooleanMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Z) return incorrect value");
  }

}

