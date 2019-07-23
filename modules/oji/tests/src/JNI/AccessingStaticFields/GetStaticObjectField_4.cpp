



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticObjectField_4)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_string", "Ljava/lang/String;");
  jchar str_chars1[]={'a', 's', 'd', 'f'};

  jstring str = env->NewString(str_chars1, 4); 
  env->SetStaticObjectField(clazz, fieldID, str);
  jstring value = (jstring)env->GetStaticObjectField(clazz, (jfieldID)100);
  if(value == NULL){
     printf("Value is NULL!!\n\nIt's correct!\n");
     return TestResult::PASS("GetStaticObjectField(fieldID is incorrect positive) return NULL");
  }else{
     return TestResult::FAIL("GetStaticObjectField(fieldID is incorrect positive) return non-NULL!!!!");
  }

}
