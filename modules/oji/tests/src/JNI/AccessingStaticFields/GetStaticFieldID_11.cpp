



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_11)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  env->SetStaticDoubleField(clazz, fieldID, 10);
  jdouble value = env->GetStaticDoubleField(clazz, fieldID);
  if(value == 10){
    return TestResult::PASS("GetStaticFieldID(all right for double) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for double) return incorrect value");
  }

}
