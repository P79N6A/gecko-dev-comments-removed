



































#include "JNIEnvTests.h"
#include "CallingInstanceMethods.h"

JNI_OJIAPITest(JNIEnv_CallNonvirtualShortMethod_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetMethodID_METHOD("Test1", "Test1_method_short", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)S");
  char *path = "asdf";
  jstring jpath=env->NewStringUTF("sdsadasdasd");
  jvalue *args  = new jvalue[10];
  args[0].z = JNI_FALSE;
  args[1].b = MIN_JBYTE;
  args[2].c = 'a';
  args[3].s = MIN_JSHORT;
  args[4].i = 123;
  args[5].j = 0;
  args[6].f = 0;
  args[7].d = 100;
  args[8].l = jpath;
  args[9].l = NULL;
  jshort value = env->CallNonvirtualShortMethodA(obj, env->GetSuperclass(clazz), MethodID, args);
  if(value == MIN_JSHORT){
     return TestResult::PASS("CallNonvirtualShortMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)S) return correct value");
  }else{
     return TestResult::FAIL("CallNonvirtualShortMethodA for public not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)S) return incorrect value");
  }

}

