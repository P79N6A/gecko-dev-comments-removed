



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_GetFieldID_1)
{
  GET_JNI_FOR_TEST

  jclass clazz = env->FindClass("Test1"); 
  jobject obj = env->AllocObject(clazz);     
  jfieldID fieldID = env->GetFieldID(NULL, "name_int", "I"); 
  printf("fieldID = %d\n", (int)fieldID); 

  if(fieldID == NULL){
    return TestResult::PASS("GetFieldID(class=NULL) passed");
  }else{
    return TestResult::FAIL("GetFieldID(class=NULL) failed");
  }
}
