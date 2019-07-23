



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualObjectMethod_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_string", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Ljava/lang/String;");
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
  args[7].d = 100;
  args[8].l = jpath;
  args[9].l = NULL;
  jstring value = (jstring)env->CallNonvirtualObjectMethodA(obj, env->GetSuperclass(clazz), MethodID, args);
  if(env->GetStringLength(value) == 11){
     return TestResult::PASS("CallNonvirtualObjectMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Ljava/lang/String;) return correct value");
  }else{
     return TestResult::FAIL("CallNonvirtualObjectMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)Ljava/lang/String;) return incorrect value");
  }

}

