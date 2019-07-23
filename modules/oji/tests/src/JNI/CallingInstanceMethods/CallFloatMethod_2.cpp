



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallFloatMethod_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_float", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F");
  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jvalue *args  = new jvalue[10];
  args[0].z = JNI_FALSE;
  args[1].b = MIN_JBYTE;
  args[2].c = 'a';
  args[3].s = MAX_JSHORT;
  args[4].i = 123;
  args[5].j = 0;
  args[6].f = MIN_JFLOAT;
  args[7].d = 100;
  args[8].l = jpath;
  args[9].l = NULL;
  jfloat value = env->CallFloatMethodA(obj, MethodID, args);
  if(value == MIN_JFLOAT){
     return TestResult::PASS("CallFloatMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F) return correct value");
  }else{
     return TestResult::FAIL("CallFloatMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)F) return incorrect value");
  }

}

