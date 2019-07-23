



































#include "JNIEnvTests.h"
#include "ArrayOperations.h"

JNI_OJIAPITest(JNIEnv_NewObjectArray_3)
{
  GET_JNI_FOR_TEST
  jclass clazz = env->FindClass("Ljava/lang/String;");
  if(clazz == NULL) {
     printf("Class is NULL!!!!\n\n");
  }
  jmethodID methodID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
  printf("methodID = %d\n\n", (int)methodID);
  jchar str_chars[]={'T', 'e', 's', 't'};
  jstring str = env->NewString(str_chars, 4); 
  jobject obj = env->NewObject(clazz, methodID, str);
  jarray arr = env->NewObjectArray(0, clazz, obj);
  if(arr==NULL){
     printf("ARR is NULL!!\n\n");
  }
  jsize len = env->GetArrayLength(arr);
  if((int)len==0){
     return TestResult::PASS("NewObjectArray(all correct with length = 0 ) returns correct value - empty array");
  }else{
     return TestResult::FAIL("NewObjectArray(all correct with length = 0 ) returns incorrect value");
  }


}
