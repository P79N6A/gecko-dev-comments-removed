



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticIntMethod_11)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test2_method3_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I");
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
  jint value = env->CallStaticIntMethodA(clazz, MethodID, args);
  if(value == 121){
     return TestResult::PASS("CallStaticIntMethodA for public inherited from superclass method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return correct value");
  }else{
     return TestResult::FAIL("CallStaticIntMethodA for public inherited from superclass method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return incorrect value");
  }

}
