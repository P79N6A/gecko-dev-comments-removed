



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_17)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "public_static_super_object", "Ljava/lang/String;");
  jchar str_chars[]={'a', 's', 'd', 'f'};
  jstring str = env->NewString(str_chars, 4); 
  env->SetStaticObjectField(clazz, fieldID, str);
  jstring value = (jstring)env->GetStaticObjectField(clazz, fieldID);
  jboolean isCopy = TRUE;
  char* str_chars1 = (char *) env->GetStringUTFChars(value, &isCopy);
  jsize len = env->GetStringLength(value);
  if(len == 4){
    return TestResult::PASS("GetStaticFieldID(all right for super public object (string)) return correct value");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for super public object (string)) return incorrect value");
  }

}
