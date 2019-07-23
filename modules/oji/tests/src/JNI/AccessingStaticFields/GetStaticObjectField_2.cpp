



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticObjectField_2)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_string", "Ljava/lang/String;");
  jchar str_chars1[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars1, 4); 
  env->SetStaticObjectField(clazz, fieldID, str);
  jstring value = (jstring)env->GetStaticObjectField(clazz, NULL);
  if(value == NULL){
     printf("Value is NULL!!\n\nIt's correct!\n");
     return TestResult::PASS("GetStaticObjectField(fieldID == NULL) return NULL");
  }else{
     return TestResult::FAIL("GetStaticObjectField(fieldID == NULL) return non-NULL!!!!");
  }

}
