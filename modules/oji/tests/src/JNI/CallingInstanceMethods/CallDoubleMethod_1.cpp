



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallDoubleMethod_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_double", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D");
  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jvalue *args  = new jvalue[10];
  args[0].z = JNI_FALSE;
  args[1].b = MIN_JBYTE;
  args[2].c = 'a';
  args[3].s = MAX_JSHORT;
  args[4].i = 123;
  args[5].j = 0;
  args[6].f = 0;
  args[7].d = MAX_JDOUBLE;
  args[8].l = jpath;
  args[9].l = NULL;
  jdouble value = env->CallDoubleMethodA(obj, MethodID, args);
  if(value == MAX_JDOUBLE){
     return TestResult::PASS("CallDoubleMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D) return correct value");
  }else{
     return TestResult::FAIL("CallDoubleMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)D) return incorrect value");
  }

}

