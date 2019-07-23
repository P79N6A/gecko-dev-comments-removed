



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticObjectField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_string", "Ljava/lang/String;");
  jchar str_chars1[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars1, 4); 
  env->SetStaticObjectField(clazz, fieldID, str);
  jstring value = (jstring)env->GetStaticObjectField(clazz, fieldID);

  jboolean isCopy = TRUE;
  char* str_chars = (char *) env->GetStringUTFChars(value, &isCopy);
  printf("The GetStringUTFChars returned : %s\n\n", str_chars);

  jsize len = env->GetStringLength(value);
  if((str_chars != NULL) && ((int)len == 4)){
     printf("Value is correct!!\n\nIt's correct!\n");
     return TestResult::PASS("SetStaticObjectField(all right) return correct value");
  }else{
     return TestResult::FAIL("SetStaticObjectField(all right) return incorrect value!!!!");
  }

}
