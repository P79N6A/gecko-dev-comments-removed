



































#include "JNIEnvTests.h"
#include "CallingStaticMethods.h"

JNI_OJIAPITest(JNIEnv_CallStaticIntMethod_9)
{
  GET_JNI_FOR_TEST

  jclass clazz_exp = env->FindClass("Ljava/security/PrivilegedActionException;");
  IMPLEMENT_GetStaticMethodID_METHOD("Test1", "Test1_method32_static", "(ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I");
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
  jthrowable excep = env->ExceptionOccurred();
  if(value==0){
     if((excep != NULL) && (env->IsInstanceOf(excep, clazz_exp))){
       printf("Exception Occurred, it is correct!!!!\n");
       return TestResult::PASS("CallStaticIntMethodA for protected not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return correct value");
     }else{
       if(SecENV){
         return TestResult::FAIL("CallStaticIntMethodA for protected not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return incorrect value");
       }else{
         return TestResult::PASS("CallStaticIntMethodA for protected not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return correct value");
       }
     }
  }else{
    return TestResult::FAIL("CallStaticIntMethodA for protected not inherited method (sig = (ZBCSIJFDLjava/lang/String;[Ljava/lang/String;)I) return incorrect value");
  }

}
