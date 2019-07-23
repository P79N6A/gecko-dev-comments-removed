



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetObjectField_3)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_string", "Ljava/lang/String;");
  jstring value = (jstring)env->GetObjectField(obj, (jfieldID)-100);
  
  return TestResult::PASS("GetObjectField with fieldID = (jfieldID)-100 return correct value - do not crash");

}
