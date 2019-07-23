



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_SetStaticObjectField_1)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_string", "Ljava/lang/String;");
  jchar str_chars1[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars1, 4); 
  env->SetStaticObjectField(NULL, fieldID, str);
  jstring value = (jstring)env->GetStaticObjectField(clazz, fieldID);

  jboolean isCopy = TRUE;
  char* str_chars = (char *) env->GetStringUTFChars(value, &isCopy);
  printf("The GetStringUTFChars returned : %s\n\n", str_chars);

  jsize len = env->GetStringLength(value);
  if((str_chars != NULL) && ((int)len == 3)){
     printf("Value is correct!!\n\nIt's correct!\n");
     return TestResult::PASS("SetStaticObjectField(clazz == NULL) return correct value");
  }else{
     return TestResult::FAIL("SetStaticObjectField(clazz == NULL) return incorrect value!!!!");
  }

}
