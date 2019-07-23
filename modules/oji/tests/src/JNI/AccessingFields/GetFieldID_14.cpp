



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_14)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1", "name_string", "Ljava/lang/String;");
  jchar str_chars[]={'a', 's', 'd', 'f'};
  jstring str = env->NewString(str_chars, 4); 
  env->SetObjectField(obj, fieldID, str);
  jstring value = (jstring)env->GetObjectField(obj, fieldID);
  jboolean isCopy = TRUE;
  char* str_chars1 = (char *) env->GetStringUTFChars(value, &isCopy);
  jsize len = env->GetStringLength(value);
  if((fieldID != NULL) &&(len == 4)){
    return TestResult::PASS("GetFieldID(all right for object (java/lang/String)) passed");
  }else{
    return TestResult::FAIL("GetFieldID(all right for object (java/lang/String)) failed");
  }

}
