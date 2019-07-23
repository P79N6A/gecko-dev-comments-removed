



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticDoubleField_6)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  env->SetStaticDoubleField(clazz, fieldID, -100);
  printf("value = %Lf\n", env->GetStaticDoubleField(clazz, fieldID));
  if(env->GetStaticDoubleField(clazz, fieldID) == -100){
    return TestResult::PASS("SetStaticDoubleField(all correct, value == -100) set correct value to field");
  }else{
    return TestResult::FAIL("SetStaticDoubleField(all correct, value == -100) set incorrect value to field");
  }

}
