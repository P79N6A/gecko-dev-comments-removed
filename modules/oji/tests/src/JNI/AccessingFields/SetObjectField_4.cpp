



































#include "JNIEnvTests.h"
#include "AccessingFields.h"

JNI_OJIAPITest(JNIEnv_SetObjectField_4)
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

  env->SetObjectField(obj, NULL, arr);
  jintArray value = (jintArray)env->GetObjectField(obj, fieldID);
  jint val[3];
  env->GetIntArrayRegion(value, start, leng, val);
  printf("value = %d\n and value = %d\n and value = %d\n", (int)val[0], (int)val[1], (int)val[2]);
  if((val[0]==0) && (val[1]==0) && (val[2]==0)){
      return TestResult::PASS("SetObjectField(for fieldID = NULL) passed");
  }else{
      return TestResult::FAIL("SetObjectField(for fieldID = NULL) failed");
  }

}
