



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticDoubleField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_double", "D");
  jdouble value = env->GetStaticDoubleField(clazz, fieldID);
  printf("value = %Lf\n", (double)value);
  if(value == 1){ 
     return TestResult::PASS("GetStaticDoubleField(all correct) return correct value");
  }else{
     return TestResult::FAIL("GetStaticDoubleField(all correct) return incorrect value");
  }
}
