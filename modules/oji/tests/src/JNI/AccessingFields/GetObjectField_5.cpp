



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetObjectField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_string", "Ljava/lang/String;");
  jstring jpath=env->NewStringUTF("asdf");
  env->SetObjectField(obj, fieldID, jpath);
  jstring value = (jstring)env->GetObjectField(obj, fieldID);
  char* str_chars_ret = (char *) env->GetStringUTFChars(value, NULL);
  printf("value = %s\n", value);
  if(strcmp(str_chars_ret, "asdf") == 0){
    return TestResult::PASS("GetObjectField(all correct) return correct value");
  }else{
    return TestResult::FAIL("GetObjectField(all correct) return incorrect value");
  }
}
