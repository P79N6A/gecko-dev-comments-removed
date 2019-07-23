



































#include "JNIEnvTests.h"
#include "AccessingStaticFields.h"

JNI_OJIAPITest(JNIEnv_GetStaticFieldID_15)
{
  GET_JNI_FOR_TEST

  IMPLEMENT_GetStaticFieldID_METHOD("Test1", "static_name_int_arr", "[I");
  jintArray arr = env->NewIntArray(3);
  jsize start = 0;
  jsize leng = 3;
  jint buf[3];
  buf[0] = 1;
  buf[1] = 2;
  buf[2] = 3;
  env->SetIntArrayRegion(arr, start, leng, buf);
  env->SetStaticObjectField(clazz, fieldID, arr);
  jintArray value = (jintArray)env->GetStaticObjectField(clazz, fieldID);
  jint val[3];
  env->GetIntArrayRegion(value, start, leng, val);
  printf("value = %d\n and value = %d\n and value = %d\n", (int)val[0], (int)val[1], (int)val[2]);
  if((fieldID != NULL) &&(val[0] == 1) && (val[1] == 2) && (val[2] == 3)){
    return TestResult::PASS("GetStaticFieldID(all right for array) passed");
  }else{
    return TestResult::FAIL("GetStaticFieldID(all right for array) failed");
  }

}
