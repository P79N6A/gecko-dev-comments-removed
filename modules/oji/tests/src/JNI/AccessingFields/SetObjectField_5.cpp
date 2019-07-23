



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetObjectField_5)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetFieldID_METHOD("Test1","name_int_arr", "[I");
  jintArray arr = env->NewIntArray(3);
  jsize start = 0;
  jsize leng = 3;
  jint buf[3];
  buf[0] = 1;
  buf[1] = 2;
  buf[2] = 3;
  env->SetIntArrayRegion(arr, start, leng, buf);
  env->SetObjectField(obj, fieldID, NULL);
  jintArray value = (jintArray)env->GetObjectField(obj, fieldID);
  if(value==NULL){
      return TestResult::PASS("SetObjectField(object == NULL) passed");
  }else{
      return TestResult::FAIL("SetObjectField(object == NULL) failed");
  }

}
