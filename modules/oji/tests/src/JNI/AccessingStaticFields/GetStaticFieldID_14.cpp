



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_14)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_string", "Ljava/lang/String;");
  jstring value = (jstring)env->GetStaticObjectField(clazz, fieldID);
  jboolean isCopy = TRUE;
  char* str_chars1 = (char *) env->GetStringUTFChars(value, &isCopy);
  jsize len = env->GetStringLength(value);
  if(len == 3){
    return TestResult::PASS("GetStaticFieldID(all right for object (string)) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for object (string)) return incorrect value");
  }

}
